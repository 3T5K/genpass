CC = gcc
CFLAGS = -Wall -Werror -Wpedantic -Wextra -O3
TARGET = genpass

$(TARGET):
	$(CC) $(CFLAGS) main.c -o $(TARGET)

all: $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean install uninstall
