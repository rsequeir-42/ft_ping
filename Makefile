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

-include $(DEP)

.PHONY: all debug release clean fclean re