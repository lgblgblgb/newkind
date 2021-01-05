/*
 * Elite - The New Kind.
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder.
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 * rewritten for SDL2 by G.Lenart (LGB) 2019, <lgblgblgb@gmail.com>
 * No "MIDI" (music, let it be midi, or something else ...) support yet.
 *
 *
 */

/*
 * sound.c
 */

#include "etnk.h"

#include <stdlib.h>
#include "sound.h"
#include "file.h"

#define NUM_SAMPLES 14

#define SILENCE	0x80


static const char *sample_filenames[NUM_SAMPLES] = {
	"launch.wav", "crash.wav", "dock.wav", "gameover.wav", "pulse.wav", "hitem.wav", "explode.wav", "ecm.wav", "missile.wav", "hyper.wav", "incom1.wav", "incom2.wav", "beep.wav", "boop.wav"
};
static const Uint8 *sample_p[NUM_SAMPLES];
static       int    sample_s[NUM_SAMPLES];


static SDL_AudioDeviceID audio = 0;


static int play_sfx_request = -1;




static void audio_callback ( void *userdata, Uint8 *stream, int len )
{
	static const Uint8 *playing_pos = NULL;
	static const Uint8 *playing_end = NULL;
	if (ETNK_UNLIKELY(play_sfx_request >= 0 && play_sfx_request < NUM_SAMPLES && sample_p[play_sfx_request])) {
		printf("AUDIO: start to playing sample #%d\n", play_sfx_request);
		playing_pos = sample_p[play_sfx_request] + 44;	// really lame, we use WAVs as is, without even checking header or anything and assuming that it will work. wow. :-O
		playing_end = sample_p[play_sfx_request] + sample_s[play_sfx_request];
		play_sfx_request = -1;
	}
	if (!playing_pos) {
		memset(stream, SILENCE, len);
	} else {
		for (int i = 0; i < len; i++) {
			if (playing_pos) {
				stream[i] = *playing_pos;
				playing_pos++;
				if (playing_pos >= playing_end) {
					playing_pos = NULL;
					puts("AUDIO: end of sample");
				}
			} else {
				stream[i] = SILENCE;
			}
		}
	}
}



void snd_sound_startup (void)
{
	play_sfx_request = -1;
	// Init audio
	SDL_AudioSpec audio_want, audio_got;
	SDL_memset(&audio_want, 0, sizeof(audio_want));
	audio_want.freq = 22050;
	audio_want.format = AUDIO_U8;
	audio_want.channels = 1;
	audio_want.samples = 1024;
	audio_want.callback = audio_callback;
	audio_want.userdata = NULL;
	audio = SDL_OpenAudioDevice(NULL, 0, &audio_want, &audio_got, 0);
	if (audio) {
		if (audio_want.freq != audio_got.freq || audio_want.format != audio_got.format || audio_want.channels != audio_got.channels) {
			SDL_CloseAudioDevice(audio);    // forget audio, if it's not our expected format :(
			audio = 0;
			fprintf(stderr, "Audio parameter mismatches\n");
		} else {
			printf("AUDIO: initialized (#%d), %d Hz, %d channels, %d buffer sample size.\n", audio, audio_got.freq, audio_got.channels, audio_got.samples);
			// Load samples ... well, get the references/sizes from memory :)
			for (int i = 0; i < NUM_SAMPLES; i++)
				datafile_select(sample_filenames[i], &sample_p[i], &sample_s[i]);
			// Go for the audio ...
			SDL_PauseAudioDevice(audio, 0);
		}
	} else
		fprintf(stderr, "Cannot open audio: %s\n", SDL_GetError());
}
 

void snd_sound_shutdown (void)
{
	if (audio) {
		puts("AUDIO: closing");
		SDL_PauseAudioDevice(audio, 1);
		SDL_CloseAudioDevice(audio);
	}
}


void snd_play_sample (int sample_no)
{
	play_sfx_request = sample_no;
}



void snd_play_midi (int midi_no, int repeat)
{
	puts("FIXME: snd_play_midi() not implemented");
}


void snd_stop_midi (void)
{
	puts("FIXME: snd_stop_midi() not implemented");
}
