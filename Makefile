# ft_ping - build

NAME		:= ft_ping

CC			:= cc
SRCDIR		:= src
INCDIR		:= include

# Sources listed explicitly (no wildcard: an unlisted file is a deliberate signal).
SRC			:= main.c options.c error.c

# Build profile: release (default), debug or coverage. Use: make MODE=debug
MODE		?= release

# Preprocessor: include path + automatic header dependencies.
CPPFLAGS	:= -I$(INCDIR) -MMD -MP

# Warning set (project decision). Extracted into WARNINGS so the test build can
# reuse it minus -Wconversion (Criterion's assertion macros trip that one).
WARNINGS	:= -Wall -Wextra -Werror -std=gnu11 				\
			   -Wshadow -Wvla -Wstrict-prototypes -Wformat=2	\
			   -Wpedantic -Wcast-align -Wconversion

CFLAGS		:= $(WARNINGS)

LDFLAGS		:=

SANITIZE	:= -fsanitize=address,undefined -fno-omit-frame-pointer

ifeq ($(MODE),debug)
	OBJDIR	:= obj/debug
	CFLAGS	+= -Og -g3 $(SANITIZE)
	LDFLAGS	+= $(SANITIZE)
else ifeq ($(MODE),release)
	OBJDIR	:= obj/release
	CFLAGS	+= -O2 -DNDEBUG
else ifeq ($(MODE),coverage)
	OBJDIR	:= obj/coverage
	CFLAGS	+= -O0 -g --coverage -fprofile-abs-path
	LDFLAGS	+= --coverage
else
	$(error unknown MODE '$(MODE)' (use 'release', 'debug' or 'coverage'))
endif

OBJ			:= $(SRC:%.c=$(OBJDIR)/%.o)
DEP			:= $(OBJ:.o=.d)

# Quality tools the make targets rely on (format/lint/analyze/memcheck/coverage).
# Versioned names make the major version implicit (gcc-13, clang-format-19...).
QUALITY_TOOLS := gcc-13 clang-format-19 clang-tidy-19 cppcheck valgrind gcovr bear

# Quality tool executables, pinned to match the toolchain the playbook installs.
# Override on the command line if needed, e.g. make CLANG_FORMAT=clang-format-18.
CLANG_FORMAT	?= clang-format-19
CLANG_TIDY		?= clang-tidy-19
CPPCHECK		?= cppcheck
VALGRIND		?= valgrind
ANALYZER_CC		?= gcc-13
GCOV			?= gcov-13

# Criterion (unit test framework), discovered via pkg-config. Silenced so a
# missing lib doesn't spam stderr at parse time -- check-env reports it cleanly.
CRITERION_CFLAGS := $(shell pkg-config --cflags criterion 2>/dev/null)
CRITERION_LIBS	 := $(shell pkg-config --libs criterion 2>/dev/null)

# Unit tests: Criterion provides its own main(), so the test binary links the
# module objects WITHOUT main.o. Built in debug + sanitizers; warnings minus
# -Wconversion (tripped by Criterion's macros). Lives under obj/ (gitignored).
TESTDIR		:= tests/unit
TEST_SRC	:= $(wildcard $(TESTDIR)/test_*.c)
TEST_OBJDIR	:= obj/test
TEST_BIN	:= $(TEST_OBJDIR)/test_runner
LIB_SRC		:= $(filter-out main.c,$(SRC))
TEST_CFLAGS	:= $(filter-out -Wconversion,$(WARNINGS)) -Og -g3 $(SANITIZE)

# Files for the quality targets: every source+header for formatting; the
# compiled translation units for clang-tidy (headers covered via HeaderFilterRegex).
FORMAT_FILES	:= $(shell find $(SRCDIR) $(INCDIR) $(TESTDIR) \( -name '*.c' -o -name '*.h' \) 2>/dev/null)

all: $(NAME)

# Link
$(NAME): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

# Compile: order-only dependency on the profile object dir.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Profile aliases (sub-make so MODE is fixed before the graph is built).
debug:
	$(MAKE) MODE=debug

release:
	$(MAKE) MODE=release

clean:
	$(RM) -r obj

fclean: clean
	$(RM) $(NAME)

re: fclean
	$(MAKE) all

# Provisioning: install the toolchain on the current machine (Ansible).
# Touches the system (sudo) -> separate from 'all', never invoked by it.
bootstrap:
	./scripts/bootstrap.sh

# Create + install + provision a reproducible Debian trixie VM (VBoxManage +
# preseed + Ansible). Needs VirtualBox (user in 'vboxusers'); the ISO and disk
# live outside the repo. Like 'bootstrap', never invoked by 'all'.
vm:
	./scripts/create-vm.sh

# Lint the whole provisioning content (playbook + group_vars). No explicit file
# -> ansible-lint auto-discovers every Ansible file in the directory. Prefix
# /usr/bin so the 'ansible' resolved via PATH is apt's (matching the imported
# module), avoiding "Ansible CLI and python module versions do not match".
lint-playbook:
	cd provisioning && PATH=/usr/bin:$$PATH ansible-lint

# Preflight: READ-ONLY check that the quality tools are present (no install, no
# sudo). Fails with a clear message if one is missing. Prerequisite of 'check';
# never a prerequisite of 'all'.
check-env:
	@missing=0; \
	for t in $(QUALITY_TOOLS); do \
		command -v $$t >/dev/null 2>&1 || { echo "  missing: $$t"; missing=1; }; \
	done; \
	pkg-config --exists criterion 2>/dev/null || { echo "  missing: criterion (libcriterion-dev)"; missing=1; }; \
	if [ $$missing -ne 0 ]; then \
		echo "Incomplete toolchain -> run 'make bootstrap'."; exit 1; \
	fi; \
	echo "check-env: all quality tools present."

# --- Formatting --------------------------------------------------------------
# format applies in place; format-check only verifies (pre-commit hook, 'check').
format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

# --- Static analysis (lint) --------------------------------------------------
# clang-tidy needs a compilation database (exact flags per file). Bear records it
# by observing a real build; regenerated whenever the Makefile changes.
compile_commands.json: Makefile
	$(MAKE) fclean
	bear -- $(MAKE) MODE=debug

compile-db: compile_commands.json

# clang-tidy on the compiled translation units (headers via HeaderFilterRegex).
tidy: compile_commands.json
	$(CLANG_TIDY) -p . $(addprefix $(SRCDIR)/,$(SRC))

# cppcheck: an independent engine, complements clang-tidy. --template=gcc gives
# the same 'file:line:col: severity:' shape as gcc and clang-tidy, so a single
# CI problem matcher annotates all three inline in the PR diff.
cppcheck:
	$(CPPCHECK) --enable=warning,style,performance,portability --std=c11 \
		--language=c --template=gcc --error-exitcode=1 --inline-suppr \
		-I$(INCDIR) $(SRCDIR)

lint: tidy cppcheck

# --- Deeper analysis (kept out of 'check': slower / privileged) --------------
# GCC's analyzer. -fsyntax-only runs it without emitting objects, so it scales
# to several files. Strict: analyzer warnings are errors via CFLAGS (-Werror).
analyze:
	$(ANALYZER_CC) -I$(INCDIR) $(CFLAGS) -fanalyzer -fsyntax-only \
		$(addprefix $(SRCDIR)/,$(SRC))

# valgrind: catches what ASan does not (uninitialised reads). On a binary
# WITHOUT capabilities -- valgrind refuses a setcap'd executable. Driven through
# argument-only paths that exit 0 (help, then a plain parse); the network engine
# and the error paths that exit non-zero are covered by other targets.
memcheck: $(NAME)
	$(VALGRIND) --leak-check=full --error-exitcode=42 \
		--errors-for-leak-kinds=definite,possible ./$(NAME) --help >/dev/null
	$(VALGRIND) --leak-check=full --error-exitcode=42 \
		--errors-for-leak-kinds=definite,possible ./$(NAME) localhost

# Coverage: structure only; the measurement is wired once there are unit tests
# exercising real modules (a .gcda written under sudo would be root-owned, so
# coverage runs on the unit tests, not the privileged binary). GCOV matches gcc-13.
coverage:
	@echo "coverage: deferred until modules exist -- see DEFERRED.md ($(GCOV))"
	@true

# --- Unit tests (Criterion) --------------------------------------------------
# Test binary = module objects (WITHOUT main.o) + test sources + -lcriterion,
# compiled in debug/ASan so 'make test' also catches memory bugs in the logic.
$(TEST_OBJDIR):
	mkdir -p $(TEST_OBJDIR)

$(TEST_BIN): $(addprefix $(SRCDIR)/,$(LIB_SRC)) $(TEST_SRC) | $(TEST_OBJDIR)
	$(CC) -I$(INCDIR) $(CRITERION_CFLAGS) $(TEST_CFLAGS) $^ $(CRITERION_LIBS) -o $@

# 'test' also writes a JUnit report under reports/ (gitignored) for CI to upload.
# Criterion's provider is named 'xml', not 'junit'; passing a file keeps the
# console output and the non-zero exit on failure. LSAN_OPTIONS points ASan's
# leak checker at a suppression for Criterion's own internal leaks (its argument
# handling strdup's the output spec and never frees it), so leaks in OUR code
# still fail the build while the framework's do not.
test: $(TEST_BIN)
	mkdir -p reports
	LSAN_OPTIONS=suppressions=$(TESTDIR)/lsan.supp \
		CRITERION_OUTPUTS="xml:reports/junit.xml" ./$(TEST_BIN)

# --- Conformance (black-box) -------------------------------------------------
# Runs the real ft_ping binary and compares its CLI output to frozen snapshots
# (established against the inetutils-2.0 etalon). A self-contained shell harness
# -- nothing to install or vendor, so it runs anywhere bash does (the 42 repo
# takes no submodules). LC_ALL=C so glibc never translates argp. `check` runs it
# on the debug/ASan build it just made, exercising the binary under ASan along
# the CLI paths (this absorbs the former run-asan placeholder). Snapshots are
# regenerated by generate-snapshots.sh.
CONFORMANCE := tests/conformance/conformance.sh

conformance:
	@[ -x ./$(NAME) ] || { echo "conformance: ./$(NAME) absent -- 'make debug' first"; exit 1; }
	LC_ALL=C $(CONFORMANCE)

# --- Single gate + hook activation -------------------------------------------
# One door, run identically locally and (later) in CI. Fail-fast, left to right.
check: check-env format-check lint debug test conformance

# Activate the versioned git hooks (run once after clone).
hooks:
	git config core.hooksPath .githooks

# --- Development journal ------------------------------------------------------
# Serve the mdBook journal locally with live reload (for writing/previewing).
# Needs mdBook in PATH, installed on demand -- it is NOT part of the toolchain
# checked by check-env. The generated book/ is gitignored.
book-serve:
	mdbook serve --open

-include $(DEP)

.PHONY: all debug release clean fclean re bootstrap vm lint-playbook check-env \
		format format-check compile-db tidy cppcheck lint analyze memcheck \
		coverage test conformance check hooks book-serve
