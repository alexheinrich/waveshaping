#include <stdio.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <rtmidi/rtmidi_c.h>

#define SAMPLING_RATE 44100
#define PI 3.14159265
#define TWOPI PI * 2

#define BUF_SZ 16
#define NUM_OSC 8

typedef enum {
	PRESSED,
	RELEASED,
	RESETTING,
	OFF
} notestatus_t;

typedef struct {
	double twopiovrsr;
	double curfreq;
	double stagedfreq;
	double curphase;
	double incr;
} oscil_t;

typedef struct {
	notestatus_t note_status;
	double current_value;
	double increment;
	double attack;
	double release;
	double max_volume;
} env_t;

oscil_t *oscillators[NUM_OSC];
oscil_t *modulators[NUM_OSC];
oscil_t *lfos[NUM_OSC];
env_t *envelopes[NUM_OSC];

extern void mid_init(void);
extern void mid_quit(void);
