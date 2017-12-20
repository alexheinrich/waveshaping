GCC := gcc
SDL2 := /opt/sdl2

SDL2_INC := $(SDL2)/include
SDL2_LIB := $(SDL2)/lib

LIBS := $(SDL2_LIB)/libSDL2.a \
		-framework AppKit \
		-framework AudioToolbox \
		-framework Carbon \
		-framework CoreMIDI \
		-framework CoreAudio \
		-framework CoreFoundation \
		-framework CoreGraphics \
		-framework CoreVideo \
		-framework ForceFeedback \
		-framework IOKit \
		-l iconv

FLAGS := -O2 -Wall -Wextra -I $(SDL2_INC)

%.o: %.c
	$(GCC) $(FLAGS) -c -o $@ $< $(FLAGS)

sine: sine.o
	$(GCC) -o $@ $< $(LIBS)

clean:
	rm -f *.o sine
