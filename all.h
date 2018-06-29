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

#define NUM_VOICE_OSC 4
#define NUM_VOICE_IND 6

typedef struct {
	double twopiovrsr;
	double curfreq;
	double stagedfreq;
	double curphase;
	double lastphase;
	double incr;
	double modratio;
} oscil_t;

typedef struct {
	notestatus_t note_status;
	double current_value;
	double increment;
	double attack;
	double release;
	double max_volume;
} env_t;

typedef struct {
	oscil_t osc[NUM_VOICE_OSC];
	oscil_t ind[NUM_VOICE_IND];
} voice_t;

oscil_t *oscillators[NUM_OSC];
oscil_t *modulators[NUM_OSC];
oscil_t *lfos[NUM_OSC];
env_t *envelopes[NUM_OSC];

voice_t *voices[NUM_OSC];

extern void mid_init(void);
extern void mid_quit(void);

extern void voice_set_frq(voice_t *voice, double frq);

extern double calc_tick(voice_t *voice);
extern double calc_volume(env_t *p_env);
extern void fm_init(void);
