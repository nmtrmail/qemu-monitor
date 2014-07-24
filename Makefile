TARGET = main
.DEFAULT_GOAL = all

CFLAGS = -Wall -Werror
DL = -lncurses -lpthread

OUTDIR = build
SRCDIR = src
INCDIR = include
INCLUDES = $(addprefix -I,$(INCDIR))

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

clean:
	rm -rf $(OUTDIR) main
