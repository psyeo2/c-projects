CC = gcc
CFLAGS = -Wall -Wextra

TARGET = bin/socket
BUILD_DIR = build

SRC = socket.c \
      http_parser.c \
      router.c \
      routers/ping.c \
      routers/users.c

OBJ = $(SRC:%.c=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) bin

.PHONY: all clean
