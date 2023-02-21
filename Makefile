MODULENAME = pg_ai
MODULE_big = pg_ai
EXTENSION = pg_ai
SRCDIR = src
OBJDIR = obj
SQLDIR = sql

MODULES = $(MODULENAME)
DATA_built = $(MODULENAME).sql

OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c))

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(MODULENAME).sql: $(SQLDIR)/$(MODULENAME)--1.0.sql
	cat $^ > $@

$(OBJDIR) $(SQLDIR):
	mkdir -p $@

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

