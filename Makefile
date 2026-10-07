VERSION := 1.0.0
TARGET  := genpass
SOURCE  := $(TARGET).c
PREFIX  := /usr/local
DIST    := $(TARGET)-$(VERSION)
CC      := cc
CFLAGS  := -Wall -Werror -Wpedantic -Wextra -O3

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

all: $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

dist:
	mkdir -p $(DIST)
	cp $(SOURCE) LICENSE Makefile README.md $(DIST)
	tar -czf $(DIST).tar.gz $(DIST)
	rm -rf $(DIST)

.PHONY: all clean install uninstall dist
