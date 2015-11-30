PREFIX = /usr
BIN = /bin
DATA = /share
BINDIR = $(PREFIX)$(BIN)
DATADIR = $(PREFIX)$(DATA)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
MANDIR = $(DATADIR)/man
MAN1DIR = $(MANDIR)/man1
LICENSES = $(DATADIR)/licenses

COMMAND = crt-calibrator
PKGNAME = crt-calibrator


LIBS = libdrm

FLAGS = -std=gnu99 -Og -g -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2  \
        -Winit-self -Wmissing-include-dirs -Wtrampolines -Wfloat-equal -Wshadow  \
        -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls            \
        -Wnested-externs -Winline -Wno-variadic-macros -Wswitch-default          \
        -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align -Wstrict-overflow   \
        -Wdeclaration-after-statement -Wundef -Wbad-function-cast -Wcast-qual    \
        -Wwrite-strings -Wlogical-op -Waggregate-return -Wstrict-prototypes      \
        -Wold-style-definition -Wpacked -Wvector-operation-performance           \
        -Wunsuffixed-float-constants -Wsuggest-attribute=const                   \
        -Wsuggest-attribute=noreturn -Wsuggest-attribute=pure                    \
        -Wsuggest-attribute=format -Wnormalized=nfkc -Wconversion                \
        -fstrict-aliasing -fstrict-overflow -fipa-pure-const -ftree-vrp          \
        -fstack-usage -funsafe-loop-optimizations -Wtraditional-conversion

LD_FLAGS = -lm $(shell pkg-config --libs $(LIBS)) $(FLAGS) $(LDFLAGS)
C_FLAGS = $(shell pkg-config --cflags $(LIBS)) $(FLAGS) $(CFLAGS) $(CPPFLAGS)

OBJS = calibrator drmgamma framebuffer gamma state



.PHONY: default
default: base info

.PHONY: all
all: base doc

.PHONY: base
base: cmd


.PHONY: cmd
cmd: bin/crt-calibrator

bin/crt-calibrator: $(foreach O,$(OBJS),obj/$(O).o)
	@mkdir -p bin
	$(CC) $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.c src/*.h
	@mkdir -p obj
	$(CC) $(C_FLAGS) -c -o $@ $<


.PHONY: doc
doc: info pdf dvi ps

.PHONY: info
info: bin/crt-calibrator.info
bin/%.info: doc/info/%.texinfo
	@mkdir -p bin
	$(MAKEINFO) $<
	mv $*.info $@

.PHONY: pdf
pdf: bin/crt-calibrator.pdf
bin/%.pdf: doc/info/%.texinfo
	@! test -d obj/pdf || rm -rf obj/pdf
	@mkdir -p bin obj/pdf
	cd obj/pdf && texi2pdf ../../"$<" < /dev/null
	mv obj/pdf/$*.pdf $@

.PHONY: dvi
dvi: bin/crt-calibrator.dvi
bin/%.dvi: doc/info/%.texinfo
	@! test -d obj/dvi || rm -rf obj/dvi
	@mkdir -p bin obj/dvi
	cd obj/dvi && $(TEXI2DVI) ../../"$<" < /dev/null
	mv obj/dvi/$*.dvi $@

.PHONY: ps
ps: bin/crt-calibrator.ps
bin/%.ps: doc/info/%.texinfo
	@! test -d obj/ps || rm -rf obj/ps
	@mkdir -p bin obj/ps
	cd obj/ps && texi2pdf --ps ../../"$<" < /dev/null
	mv obj/ps/$*.ps $@



.PHONY: install
install: install-base install-info install-man

.PHONY: install-all
install-all: install-base install-doc

.PHONY: install-base
install-base: install-cmd install-copyright


.PHONY: install-cmd
install-cmd: bin/crt-calibrator
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"


.PHONY: install-copyright
install-copyright: install-copying install-license

.PHONY: install-copying
install-copying:
	install -dm755 -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"
	install -m644 COPYING -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"
	install -m644 LICENSE -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"


.PHONY: install-doc
install-doc: install-info install-pdf install-dvi install-ps install-man

.PHONY: install-info
install-info: bin/crt-calibrator.info
	install -dm755 -- "$(DESTDIR)$(INFODIR)"
	install -m644 $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: bin/crt-calibrator.pdf
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-dvi
install-dvi: bin/crt-calibrator.dvi
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"

.PHONY: install-ps
install-ps: bin/crt-calibrator.ps
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-man
install-man: doc/man/crt-calibrator.1
	install -dm755 -- "$(DESTDIR)$(MAN1DIR)"
	install -m644 $< -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"



.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"
	-rm -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSES)/$(PKGNAME)"
	-rm -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
	-rm -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"



.PHONY: clean
clean:
	-rm -r bin obj

