CC := gcc
CFLAGS := -Wall
LIBS := -lubox -lblobmsg_json -lubus

BIN := UBMonitor
SRC := main.c src/ubus_methods.c src/helpers.c
OBJ := $(SRC:.c=.o)

INSTALL_DIR ?= /usr/local/bin

.PHONY: all
all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJ)

.PHONY: install
install: all
	install -D -m 755 $(BIN) $(INSTALL_DIR)/$(BIN)

.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_DIR)/$(BIN)