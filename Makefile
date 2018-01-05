GCC := gcc
G++ := g++
SDL2 := /opt/sdl2
RTMIDI := /opt/rtmidi

SDL2_INC := $(SDL2)/include
SDL2_LIB := $(SDL2)/lib

RTMIDI_INC := $(RTMIDI)/include
RTMIDI_LIB := $(RTMIDI)/lib

LIBS := $(SDL2_LIB)/libSDL2.a \
		$(RTMIDI_LIB)/librtmidi.a \
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

FLAGS := -O2 -Wall -Wextra -I $(SDL2_INC) -I $(RTMIDI_INC)

%.o: %.c
	$(GCC) $(FLAGS) -c -o $@ $< $(FLAGS)

sine: sine.o
	$(G++) -o $@ $< $(LIBS)

clean:
	rm -f *.o sine
