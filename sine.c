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
#define NUM_OSC 4

typedef struct {
	double twopiovrsr;
	double curfreq;
	double curphase;
	double incr;
} oscil_t;

float staged_frequencies[NUM_OSC];
int32_t current_oscillator = 0;

oscil_t *oscillators[NUM_OSC];

float frequency1 = 440.0;
float frequency2 = 440.0;
float frequency3 = 440.0;
float frequency4 = 440.0;

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

struct RtMidiWrapper *midiin;
double volume;
bool res_vol;

int32_t t_delt[NUM_OSC];
bool note_playing[NUM_OSC];


void oscil_init(oscil_t *osc, unsigned long s_rate)
{
	osc->twopiovrsr = TWOPI / (double) s_rate;
	osc->curfreq = 0.0;
	osc->curphase = 0.0;
	osc->incr = 0.0;
}

oscil_t *oscil(void)
{
	oscil_t *osc = (oscil_t*) malloc(sizeof(oscil_t));
	if (osc == NULL) {
		return NULL;
	}
	return osc;
}

double sine_tick(oscil_t* p_osc, double freq)
{
	double tick;

	tick = sin(p_osc->curphase);

	int32_t int_tick = ((int32_t) tick) * 115 + 115;

	for (uint32_t i = 1; i < 32; i++) {
		double volume;

		if (i % 3) {
			volume = 1;
		} else {
			volume = 0.25;
		}

		tick += volume * ((double) knmtab[i][int_tick] / 1023.0);
	}

	tick = tick/100.0;

	if (tick > 1.0 || tick < -1.0) {
		printf("Amplitude has exceed 1\n");
	}

	if (p_osc->curfreq != freq) {
		p_osc->curfreq = freq;
		p_osc->incr = p_osc->twopiovrsr * freq;
	}

	p_osc->curphase += p_osc->incr;

	if (p_osc->curphase >= TWOPI) {
		p_osc->curphase -= TWOPI;
	}

	if (p_osc->curphase < 0.0) {
		p_osc->curphase += TWOPI;
	}

	return sin(p_osc->curphase);
}

static double calc_volume(double time, int32_t osc_number)
{
	double attack = 5000.0;
	double sustain = 30000.0;
	double release = 40000.0;
	double max_volume = 0.64;

	if (time < attack) {
		return max_volume * (time / attack);
	} else if (time >= attack && time < attack + sustain) {
		return max_volume;
	} else if (time >= attack + sustain && time < attack + sustain + release) {
		double ret = max_volume * ((release - time + attack + sustain) / release);
		return ret;
	} else {
		note_playing[osc_number] = 0;
		return 0.0;
	}
}

static void audio_callback(void *udata, uint8_t *stream, int32_t len)
{
	(void) udata;
    float* floatStream = (float*) stream;

    SDL_memset(stream, 0, len);

    for (int32_t j = 0; j < len/4; j++) {
    		float stream_buffer = 0;

    		for (int32_t i = 0; i < NUM_OSC; i++) {
			if (note_playing[i]) {
				t_delt[i]++;
			} else {
				t_delt[i] = 0;
			}

    			double envelope_point = calc_volume((double) t_delt[i], i) * sine_tick(oscillators[i], staged_frequencies[i]);
    			stream_buffer = envelope_point + stream_buffer - envelope_point * stream_buffer;
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

	if ((uint8_t) message[2] == 0) {
		return;
	}

	double note_frequency = mid_to_hz(message[1]);
	// TODO: check if note already played

	staged_frequencies[current_oscillator] = note_frequency;
	note_playing[current_oscillator] = 1;

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
		printf("printfed to open Midi port");
	}

	rtmidi_in_set_callback(midiin, midi_callback, midiin->data);
	if(!midiin->ok) {
		printf("printfed to set Midi Callback");
	}
}

void mid_quit(void)
{
	if(midiin) {
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
    		t_delt[i] = 0;
    		note_playing[i] = 0;
    }

    wavSpec.freq = SAMPLING_RATE;
    wavSpec.channels = 1;
    wavSpec.samples = 512;
    wavSpec.callback = audio_callback;
    wavSpec.format = AUDIO_F32;
    wavSpec.userdata = NULL;

    volume = 0;
    res_vol = false;

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
