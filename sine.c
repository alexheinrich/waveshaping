#include<stdio.h>
#include<math.h>
#include<time.h>
#include<SDL2/SDL.h>
#include<rtmidi/rtmidi_c.h>
#include"knmtab.h"

#define SAMPLING_RATE 44100
#define PI 3.14159265
#define TWOPI PI * 2

typedef struct {
	double twopiovrsr;
	double curfreq;
	double curphase;
	double incr;
} oscil_t;

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

float frequency = 440.0;
struct RtMidiWrapper* midiin;
double volume;
bool res_vol;
int32_t t_delt;
oscil_t *oscil1;

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
	double val;

	val = sin(p_osc->curphase);
	int32_t i_val = val * 115 + 115;

	for (uint32_t i = 1; i < 32; i++) {
		double vol;
		if (i % 3) {
			vol = 1;
		} else {
			vol = 0.25;
		}
		val += vol * ((double) knmtab[i][i_val] / 1023.0);
	}

	val = val/100;

	if (val > 1.0 || val < -1.0) {
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

	return val;
}

static void init_volume(void)
{
	res_vol = true;
}

static void calc_volume(void)
{
	if (res_vol) {
		if (volume > 1) {
			volume = volume - 1;
		} else {
			volume = 0;
			res_vol = false;
			t_delt = 0;
		}

		return;
	}
	printf("%d t_delt\n", t_delt);
	if (t_delt < 1000) {
		volume = ((double) t_delt / 1000.0) * 64.0;
		//volume = 64;
	} else {
		volume = volume * 0.99995;
	}
}

static void audio_callback(void *udata, uint8_t *stream, int32_t len)
{
	(void) udata;
	static int count_sam = 0;

    float* floatStream = (float*) stream;

    SDL_memset(stream, 0, len);

    for (int j = 0; j < len/4; j++) {
        t_delt++;
        calc_volume();
    		floatStream[j] = volume * sine_tick(oscil1, frequency);
        count_sam++;
    }
}

static float mid_to_hz(const unsigned char message)
{
	return (float) 440 * pow(pow(2, 1.0/12.0), (uint8_t) message - 60);
}

static void midi_callback(double timeStamp, const unsigned char* message, size_t size, void *userData) {
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

	frequency = mid_to_hz(message[1]);
	printf("freq is %lf\n", frequency);

	init_volume();
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

static void init()
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init() printfed %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_AudioSpec wavSpec;
    SDL_zero(wavSpec);

    oscil1 = oscil();
    oscil_init(oscil1, SAMPLING_RATE);

    wavSpec.freq = SAMPLING_RATE;
    wavSpec.channels = 1;
    wavSpec.samples = 512;
    wavSpec.callback = audio_callback;
    wavSpec.format = AUDIO_F32;
    wavSpec.userdata = NULL;

    volume = 0;
    res_vol = false;
    t_delt = 999;

    AudioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioDevice == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice() failed %s\n", SDL_GetError());
    }

    // unpause AudioDevice
    SDL_PauseAudioDevice(AudioDevice, 0);
    
    mid_init();
}

static void quit()
{
    SDL_CloseAudioDevice(AudioDevice);
    SDL_Quit();

    free(oscil1);

    mid_quit();
}

int32_t main()
{
    init();

    while (true) {
    		// play some music on the keyboard
    }

    quit();
}
