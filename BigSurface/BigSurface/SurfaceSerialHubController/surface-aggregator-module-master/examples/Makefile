BUILD_DIR           ?= build
CFLAGS              += -Wall -Werror -Wextra
MKDIR               := mkdir

EXAMPLES_SRC := $(wildcard *.c)
EXAMPLES_BIN := $(patsubst %.c,$(BUILD_DIR)/%,$(EXAMPLES_SRC))


all: $(EXAMPLES_BIN)

clean:
	rm -f $(EXAMPLES_BIN)

distclean: clean
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/%: %.c
	@$(MKDIR) -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean distclean
