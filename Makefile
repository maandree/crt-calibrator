.POSIX:

CONFIGFILE = config.mk
include $(CONFIGFILE)

OBJ =\
	calibrator.o\
	drmgamma.o\
	framebuffer.o\
	gamma.o\
	state.o

HDR = common.h


all: crt-calibrator
$(OBJ): $(HDR)

crt-calibrator: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) -c -o $@ $< $(CFLAGS) $(CPPFLAGS)

install: crt-calibrator
	mkdir -p -- "$(DESTDIR)$(PREFIX)/bin"
	mkdir -p -- "$(DESTDIR)$(MANPREFIX)/man1"
	cp -- crt-calibrator "$(DESTDIR)$(PREFIX)/bin/"
	cp -- crt-calibrator.1 "$(DESTDIR)$(MANPREFIX)/man1/"

uninstall:
	-rm -- "$(DESTDIR)$(PREFIX)/bin/crt-calibrator"
	-rm -- "$(DESTDIR)$(MANPREFIX)/man1/crt-calibrator.1"

clean:
	-rm -rf -- crt-calibrator *.o *.su

.SUFFIXES:
.SUFFIXES: .o .c

.PHONY: all install uninstall clean
