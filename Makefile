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



.PHONY: all
all: bin/crt-calibrator

bin/crt-calibrator: $(foreach O,$(OBJS),obj/$(O).o)
	@mkdir -p bin
	$(CC) $(LD_FLAGS) -o $@ $^

obj/%.o: src/%.c src/*.h
	@mkdir -p obj
	$(CC) $(C_FLAGS) -c -o $@ $<


.PHONY: clean
clean:
	-rm -r bin obj

