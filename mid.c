#include "all.h"

struct RtMidiWrapper *midiin;

int32_t current_oscillator;

static float note_to_hz(const unsigned char message)
{
	return (float) 440 * pow(pow(2, 1.0/12.0), (uint8_t) message - 60);
}

static void midi_callback(double timeStamp, const uint8_t *message, void *userData)
{
	(void) timeStamp;
	(void) userData;

	for (uint8_t i = 0; i < sizeof(message); i++) {
		printf("Message %i %u ", i, (uint8_t) message[i]);
	}

	printf("\n");

	double note_frequency = note_to_hz(message[1]);

	if ((uint8_t) message[2] == 0) {
		for (int32_t i = 0; i < NUM_OSC; i++) {
			if (voices[i]->osc[0].stagedfreq == note_frequency) {
				envelopes[i]->note_status = RELEASED;
				envelopes[i]->increment = envelopes[i]->current_value / envelopes[i]->release * -1.0;
			}
		}

		return;
	}

	for (int32_t i = 0; i < NUM_OSC; i++) {
		if (voices[i]->osc[0].stagedfreq == note_frequency) {
			envelopes[i]->note_status = RESETTING;
			envelopes[i]->increment = envelopes[i]->current_value / 200.0 * -1.0;
			return;
		}
	}

	oscillators[current_oscillator]->stagedfreq = note_frequency;
	voice_set_frq(voices[current_oscillator], note_frequency);

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
	if (!midiin->ok) {
		printf("failed to open MIDI port");
	}

	rtmidi_in_set_callback(midiin, midi_callback, midiin->data);
	if (!midiin->ok) {
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
