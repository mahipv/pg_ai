MODULENAME = pg_ai
MODULE_big = pg_ai
EXTENSION = pg_ai
SRCDIR = src
OBJDIR = obj
SQLDIR = sql

MODULES = $(MODULENAME)
DATA = $(SQLDIR)/$(MODULENAME)--0.0.1.sql

CFLAGS = -Wall -O2 -g
SHLIB_LINK = -lcurl

# Find all subdirectories of SRCDIR
SRCDIRS = $(shell find $(SRCDIR) -type d)
OBJDIRS = $(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(SRCDIRS))

# Find all C source files in SRCDIR and its subdirectories
SRCS = $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
# Convert source file paths to object file paths
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# Build the shared library
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR) $(OBJDIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -I ./$(SRCDIR) -c $< -o $@

$(OBJDIR) $(OBJDIRS):
	mkdir -p $@

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)