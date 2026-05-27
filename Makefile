# ft_ping - build

NAME		:= ft_ping

CC			:= cc
SRCDIR		:= src
INCDIR		:= include

# Sources listed explicitly (no wildcard: an unlisted file is a deliberate signal).
SRC			:= main.c

# Build profile: release (default) or debug. Use: make MODE=debug (or: make debug)
MODE		?= release

# Preprocessor: include path + automatic header dependencies.
CPPFLAGS	:= -I$(INCDIR) -MMD -MP

# Compiler: maximal warning set (project decision).
CFLAGS		:= -Wall -Wextra -Werror -std=gnu11 				\
			   -Wshadow -Wvla -Wstrict-prototypes -Wformat=2	\
			   -Wpedantic -Wcast-align -Wconversion

LDFLAGS		:=

SANITIZE	:= -fsanitize=address,undefined -fno-omit-frame-pointer

ifeq ($(MODE),debug)
	OBJDIR	:= obj/debug
	CFLAGS	+= -Og -g3 $(SANITIZE)
	LDFLAGS	+= $(SANITIZE)
else ifeq ($(MODE),release)
	OBJDIR	:= obj/release
	CFLAGS	+= -O2 -DNDEBUG
else
	$(error unknown MODE '$(MODE)' (use 'release' or 'debug'))
endif

OBJ			:= $(SRC:%.c=$(OBJDIR)/%.o)
DEP			:= $(OBJ:.o=.d)

# Quality tools the make targets rely on (format/lint/analyze/memcheck/coverage).
# Versioned names make the major version implicit (gcc-13, clang-format-19...).
QUALITY_TOOLS := gcc-13 clang-format-19 clang-tidy-19 cppcheck valgrind gcovr bear

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
# sudo). Fails with a clear message if one is missing. Will gate the quality
# targets later; never a prerequisite of 'all'.
check-env:
	@missing=0; \
	for t in $(QUALITY_TOOLS); do \
		command -v $$t >/dev/null 2>&1 || { echo "  missing: $$t"; missing=1; }; \
	done; \
	if [ $$missing -ne 0 ]; then \
		echo "Incomplete toolchain -> run 'make bootstrap'."; exit 1; \
	fi; \
	echo "check-env: all quality tools present."

-include $(DEP)

.PHONY: all debug release clean fclean re bootstrap lint-playbook check-env
