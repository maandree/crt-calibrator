PREFIX    = /usr
MANPREFIX = $(PREFIX)/share/man

CPPFLAGS  = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700
CFLAGS    = -std=c99 -Wall $$(pkg-config --cflags libdrm)
LDFLAGS   = -lm $$(pkg-config --libs libdrm)
