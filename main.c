#include "all.h"

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

static void audio_callback(void *udata, uint8_t *stream, int32_t len)
{
	(void) udata;
    float* floatStream = (float*) stream;

    SDL_memset(stream, 0, len);

    for (int32_t j = 0; j < len/4; j++) {
    		float stream_buffer = 0;

    		for (int32_t i = 0; i < NUM_OSC; i++) {
    			double osc_sample = calc_volume(envelopes[i]) * calc_tick(voices[i]);
    			stream_buffer += osc_sample / NUM_OSC;
    		}

    		floatStream[j] = (float) stream_buffer;
    }
}

static void init(void)
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init() printfed %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_AudioSpec wavSpec;
    SDL_zero(wavSpec);

    fm_init();

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

    for (int32_t i = 0; i < NUM_OSC; i++) {
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
