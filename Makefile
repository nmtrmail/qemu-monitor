TARGET = qemu-monitor
.DEFAULT_GOAL = all

CFLAGS = -Wall -Werror
DL = -lncurses -lpthread

PREFIX := /usr/local/bin

OUTDIR = build
SRCDIR = src
INCDIR = include
INCLUDES = $(addprefix -I,$(INCDIR))
IPCPATH = tools/fetcher

SRC = $(wildcard $(addsuffix /*.c,$(SRCDIR)))
OBJ := $(addprefix $(OUTDIR)/,$(patsubst %.s,%.o,$(SRC:.c=.o)))

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "    LD      "$@
	@gcc $(CFLAGS) -o $@ $^ $(DL)


$(OUTDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "    CC      "$@
	@gcc $(CFLAGS) -o $@ -c $(INCLUDES) $<

install: $(TARGET)
	@echo "  INSTALL   "$(TARGET)
	@install $(TARGET) $(PREFIX)/$(TARGET)

clean:
	rm -rf $(OUTDIR) $(TARGET) $(IPCPATH)
