#include <stdio.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <rtmidi/rtmidi_c.h>
#include "knmtab.h"

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

int32_t current_oscillator = 0;

oscil_t *oscillators[NUM_OSC];
oscil_t *modulators[NUM_OSC];
oscil_t *lfos[NUM_OSC];
env_t *envelopes[NUM_OSC];

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

struct RtMidiWrapper *midiin;

oscil_t *oscil(void)
{
	oscil_t *osc = (oscil_t*) malloc(sizeof(oscil_t));
	if (osc == NULL) {
		return NULL;
	}

	return osc;
}

void oscil_init(oscil_t *osc, unsigned long s_rate)
{
	osc->twopiovrsr = TWOPI / (double) s_rate;
	osc->curfreq = 0.0;
	osc->curphase = 0.0;
	osc->incr = 0.0;
}

env_t *env(void)
{
	env_t *env = (env_t*) malloc(sizeof(env_t));
	if (env == NULL) {
		return NULL;
	}

	return env;
}

void env_init(env_t *env)
{
	env->note_status = OFF;
	env->current_value = 0.0;
	env->increment = 0.0;
	env->attack = 5000.0;
	env->release = 40000.0;
	env->max_volume = 0.64;
}

void oscil_phase_incr(oscil_t* p_osc)
{
	if (p_osc->curfreq != p_osc->stagedfreq) {
		p_osc->curfreq = p_osc->stagedfreq;
		p_osc->incr = p_osc->twopiovrsr * p_osc->stagedfreq;
	}

	p_osc->curphase += p_osc->incr;

	if (p_osc->curphase >= TWOPI) {
		p_osc->curphase -= TWOPI;
	}

	if (p_osc->curphase < 0.0) {
		p_osc->curphase += TWOPI;
	}
}

double calc_tick(oscil_t* p_osc, oscil_t* p_mod, oscil_t* p_lfo)
{
	oscil_phase_incr(p_osc);
	oscil_phase_incr(p_mod);
	oscil_phase_incr(p_lfo);

	return cos(p_osc->curphase + (4 + 2 * sin(p_lfo->curphase)) * sin(p_mod->curphase));
}

static double calc_volume(env_t* p_env)
{
	double ret = 0.0;

	if (p_env->note_status == PRESSED && p_env->current_value >= p_env->max_volume) {
		p_env->increment = 0.0;
	}

	p_env->current_value += p_env->increment;

	if (p_env->current_value <= 0.0) {
		if (p_env->note_status == RESETTING) {
			p_env->note_status = PRESSED;
			p_env->increment = p_env->max_volume / p_env->attack;
		}
		else {
			p_env->note_status = OFF;
			p_env->increment = 0.0;
		}

		p_env->current_value = 0.0;
	}

	ret = p_env->current_value;

	if (ret > 64.0 || ret < 0.0) {
		printf("ret %f\n", ret);
		ret = 0.0;
	}

	return ret;
}

static void audio_callback(void *udata, uint8_t *stream, int32_t len)
{
	(void) udata;
    float* floatStream = (float*) stream;

    SDL_memset(stream, 0, len);

    for (int32_t j = 0; j < len/4; j++) {
    		float stream_buffer = 0;

    		for (int32_t i = 0; i < NUM_OSC; i++) {
    			double envelope_point = calc_volume(envelopes[i]) * calc_tick(oscillators[i], modulators[i], lfos[i]);
    			stream_buffer += envelope_point / NUM_OSC;
    		}

    		floatStream[j] = (float) stream_buffer;
    }
}

static float mid_to_hz(const unsigned char message)
{
	return (float) 440 * pow(pow(2, 1.0/12.0), (uint8_t) message - 60);
}

static void midi_callback(double timeStamp, const unsigned char* message, size_t size, void *userData)
{
	(void) userData;
	(void) timeStamp;
	(void) size;

	for (uint8_t i = 0; i < sizeof(message); i++) {
		printf("Message %i %u ", i, (uint8_t) message[i]);
	}

	printf("\n");

	double note_frequency = mid_to_hz(message[1]);

	if ((uint8_t) message[2] == 0) {
		for (int32_t i = 0; i < NUM_OSC; i++) {
			if (oscillators[i]->stagedfreq == note_frequency) {
				envelopes[i]->note_status = RELEASED;
				envelopes[i]->increment = envelopes[i]->current_value / envelopes[i]->release * -1.0;
			}
		}

		return;
	}

	for (int32_t i = 0; i < NUM_OSC; i++) {
		if (oscillators[i]->stagedfreq == note_frequency) {
			envelopes[i]->note_status = RESETTING;
			envelopes[i]->increment = envelopes[i]->current_value / 200.0 * -1.0;
			return;
		}
	}

	oscillators[current_oscillator]->stagedfreq = note_frequency;
	if (envelopes[current_oscillator]->note_status != OFF) {
		envelopes[current_oscillator]->note_status = RESETTING;
		envelopes[current_oscillator]->increment = 0.0;
	} else {
		envelopes[current_oscillator]->note_status = PRESSED;
		envelopes[current_oscillator]->increment = envelopes[current_oscillator]->max_volume / envelopes[current_oscillator]->attack;
	}

	current_oscillator = (current_oscillator + 1) % NUM_OSC;
}

void mid_init(void)
{
	printf("mid init");

	midiin = rtmidi_in_create_default();
	printf("%p", midiin->ptr);

	uint32_t portcount = rtmidi_get_port_count(midiin);
	printf("%d", midiin->ok);

	if (portcount == 0) {
		midiin = NULL;
		printf("no midi ports\n");
		return;
	}

	for (uint32_t i = 0; i < portcount; i++) {
		printf("Port %d: %s", i, rtmidi_get_port_name(midiin, i));
	}

	rtmidi_open_port(midiin, 0, rtmidi_get_port_name(midiin, 0));
	if(!midiin->ok) {
		printf("failed to open MIDI port");
	}

	rtmidi_in_set_callback(midiin, midi_callback, midiin->data);
	if(!midiin->ok) {
		printf("failed to set MIDI Callback");
	}
}

void mid_quit(void)
{
	if (midiin) {
		rtmidi_close_port(midiin);
		rtmidi_in_free(midiin);
	}

	printf("mid quit");
}

static void init(void)
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init() printfed %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_AudioSpec wavSpec;
    SDL_zero(wavSpec);

    for(int32_t i = 0; i < NUM_OSC; i++) {
    		oscillators[i] = oscil();
    		oscil_init(oscillators[i], SAMPLING_RATE);
    		oscillators[i]->stagedfreq = 440.0;

    		modulators[i] = oscil();
    		oscil_init(modulators[i], SAMPLING_RATE);
    		modulators[i]->stagedfreq = 330.0;

    		lfos[i] = oscil();
    		oscil_init(lfos[i], SAMPLING_RATE);
    		lfos[i]->stagedfreq = 0.1;

    		envelopes[i] = env();
    		env_init(envelopes[i]);
    }

    wavSpec.freq = SAMPLING_RATE;
    wavSpec.channels = 1;
    wavSpec.samples = 512;
    wavSpec.callback = audio_callback;
    wavSpec.format = AUDIO_F32;
    wavSpec.userdata = NULL;

    AudioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioDevice == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice() failed %s\n", SDL_GetError());
    }

    // unpause AudioDevice
    SDL_PauseAudioDevice(AudioDevice, 0);
    
    mid_init();
}

static void quit(void)
{
    SDL_CloseAudioDevice(AudioDevice);
    SDL_Quit();

    for(int32_t i = 0; i < NUM_OSC; i++) {
    		free(oscillators[i]);
    		free(modulators[i]);
    		free(lfos[i]);
    }

    mid_quit();
}

int32_t main(void)
{
    init();

    while (true) {
    		// play some music on the keyboard
    }

    quit();
}
