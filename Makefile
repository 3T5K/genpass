VERSION := 1.0.2

CC ?= cc

CFLAGS ?= -Werror -O3
CFLAGS += -Wall -Wpedantic -Wextra

CPPFLAGS ?=
CPPFLAGS += -DPRJVER=\"$(VERSION)\"

TARGET := genpass
SOURCE := $(TARGET).c
DIST   := $(TARGET)-$(VERSION)
PREFIX ?= /usr/local

$(TARGET): $(SOURCE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE) -o $(TARGET)

all: $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

dist:
	mkdir -p $(DIST)
	cp $(SOURCE) LICENSE Makefile README.md $(DIST)
	tar -czf $(DIST).tar.gz $(DIST)
	rm -rf $(DIST)

.PHONY: all clean install uninstall dist
