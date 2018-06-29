#include "all.h"
#include "knmtab.h"

static voice_t *voice(void)
{
	voice_t *voice = (voice_t*) malloc(sizeof(voice_t));
	if (voice == NULL) {
		return NULL;
	}

	return voice;
}

static oscil_t *oscil(void)
{
	oscil_t *osc = (oscil_t*) malloc(sizeof(oscil_t));
	if (osc == NULL) {
		return NULL;
	}

	return osc;
}

static void oscil_init(oscil_t *osc, unsigned long s_rate)
{
	osc->twopiovrsr = TWOPI / (double) s_rate;
	osc->curfreq = 0.0;
	osc->curphase = 0.0;
	osc->lastphase = 0.0;
	osc->incr = 0.0;
}

static void oscil_p_inc(oscil_t *p_osc)
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

	p_osc->lastphase = p_osc->curphase;
}

void voice_set_frq(voice_t *voice, double frq)
{
	for (int32_t c = 0; c < NUM_VOICE_OSC; c++) {
		voice->osc[c].stagedfreq = frq;
		printf("stagedfreq %f\n", voice->osc[c].stagedfreq);
		printf("curfreq %f\n", voice->osc[c].curfreq);
	}
}

double calc_tick(voice_t *voice)
{
	for (int32_t c = 0; c < NUM_VOICE_OSC; c++) {
		oscil_p_inc(&(voice->osc[c]));
	}

	for (int32_t c = 0; c < NUM_VOICE_IND; c++) {
		oscil_p_inc(&(voice->ind[c]));
	}

	//return sin(voice->osc[0].curphase);
	return cos(voice->osc[0].curphase + (5 + sin(voice->ind[0].curphase)) * sin(voice->osc[1].curphase));
}

static env_t *env(void)
{
	env_t *env = (env_t*) malloc(sizeof(env_t));
	if (env == NULL) {
		return NULL;
	}

	return env;
}

static void env_init(env_t *env)
{
	env->note_status = OFF;
	env->current_value = 0.0;
	env->increment = 0.0;
	env->attack = 5000.0;
	env->release = 40000.0;
	env->max_volume = 0.64;
}

double calc_volume(env_t *p_env)
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

void fm_init(void)
{
    for (int32_t i = 0; i < NUM_OSC; ++ i) {
    	voices[i] = voice();

    	for (int32_t c = 0; c < NUM_VOICE_OSC; ++ c) {
    		oscil_init(&(voices[i]->osc[c]), SAMPLING_RATE);
    	}

		for (int32_t c = 0; c < NUM_VOICE_IND; ++ c) {
			oscil_init(&(voices[i]->ind[c]), SAMPLING_RATE);

			voices[i]->ind[0].stagedfreq = 0.1 * c;
			voices[i]->ind[1].stagedfreq = 0.01 * c;
			voices[i]->ind[2].stagedfreq = 0.05 * c;
			voices[i]->ind[3].stagedfreq = 0.01 * c;
		}

		oscillators[i] = oscil();
		oscil_init(oscillators[i], SAMPLING_RATE);
		oscillators[i]->stagedfreq = 440.0;
		oscillators[i]->modratio = 4;

		modulators[i] = oscil();
		oscil_init(modulators[i], SAMPLING_RATE);
		modulators[i]->stagedfreq = oscillators[i]->stagedfreq * oscillators[i]->modratio;

		lfos[i] = oscil();
		oscil_init(lfos[i], SAMPLING_RATE);
		lfos[i]->stagedfreq = 0.1;

		envelopes[i] = env();
		env_init(envelopes[i]);
    }
}
