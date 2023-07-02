MODULENAME = pg_ai
MODULE_big = pg_ai
EXTENSION = pg_ai
SRCDIR = src
OBJDIR = obj
SQLDIR = sql

MODULES = $(MODULENAME)
DATA = $(SQLDIR)/$(MODULENAME)--1.0.sql

CXXFLAGS = -Wall -O2 -g
SHLIB_LINK = -lcurl

# Find all subdirectories of SRCDIR
SRCDIRS = $(shell find $(SRCDIR) -type d)

# Find all C source files in SRCDIR and its subdirectories
SRCS = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
# Convert source file paths to object file paths
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

