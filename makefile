CC     = gcc
CFLAGS = -Wall -Werror -O2

INCLUDEDIR = include
SRCDIR     = src

BUILDDIR = build
OBJDIR   = $(BUILDDIR)/obj

CLI_OBJDIR = $(OBJDIR)/cli
CLI_SRCDIR = $(SRCDIR)/cli

DAEMON_OBJDIR = $(OBJDIR)/daemon
DAEMON_SRCDIR = $(SRCDIR)/daemon

CLI_SRC = $(wildcard $(CLI_SRCDIR)/*.c)
CLI_OBJ = $(addprefix $(CLI_OBJDIR)/, $(patsubst %.c, %.o, $(notdir $(CLI_SRC))))

DAEMON_SRC = $(wildcard $(DAEMON_SRCDIR)/*.c)
DAEMON_OBJ = $(addprefix $(DAEMON_OBJDIR)/, $(patsubst %.c, %.o, $(notdir $(DAEMON_SRC))))

.PHONY: all cli daemon clean

all: cli daemon

cli: $(CLI_OBJ)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/cli $(CLI_OBJ)

daemon: $(DAEMON_OBJ)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/daemon $(DAEMON_OBJ) -lcurl

$(CLI_OBJDIR)/%.o: $(CLI_SRCDIR)/%.c
	mkdir -p $(CLI_OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(INCLUDEDIR)

$(DAEMON_OBJDIR)/%.o: $(DAEMON_SRCDIR)/%.c
	mkdir -p $(DAEMON_OBJDIR)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(INCLUDEDIR) -lcurl

clean:
	rm -rf $(BUILDDIR)
