#include "all.h"
#include "knmtab.h"

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

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

static void init(void)
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init() printfed %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_AudioSpec wavSpec;
    SDL_zero(wavSpec);

    for (int32_t i = 0; i < NUM_OSC; i++) {
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
