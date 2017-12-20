#include<stdio.h>
#include<SDL2/SDL.h>

SDL_AudioDeviceID AudioDevice;
SDL_AudioSpec have;

static uint8_t *audio_pos; // global pointer to the audio buffer to be played
static uint32_t audio_len; // remaining length of the sample we have to play

uint8_t *wav_buffer; // buffer containing our audio file
uint32_t wav_length; // length of our sample

static void audioCallback(void *udata, uint8_t *stream, int32_t len)
{
    if (audio_len == 0) return;
    
    len = ( len > audio_len ? audio_len : len );
    printf("Length %u\n", len);
    printf("audio len %u\n", audio_len);
    printf("Audio pos %u\n", audio_pos);
    SDL_memset(stream, 0, len);
    SDL_MixAudioFormat(stream, audio_pos, have.format, len, SDL_MIX_MAXVOLUME/2);

    audio_pos += len;
    audio_len -= len;
}

static void init()
{
    if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL_Init() failed %s\n", SDL_GetError());
        exit(1);
    }
    
    SDL_AudioSpec wavSpec;
    SDL_zero(wavSpec);

    if (SDL_LoadWAV("organfinale.wav", &wavSpec, &wav_buffer, &wav_length) == NULL){
        fprintf(stderr, "SDL_LoadWAV() failed %s\n", SDL_GetError());
    }

    //wavSpec.freq = 44100;
    //wavSpec.channels = 2;
    //wavSpec.samples = 4096;
    wavSpec.callback = audioCallback;
    wavSpec.userdata = NULL;

    audio_pos = wav_buffer; // copy sound buffer
    audio_len = wav_length; // copy file length

    AudioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (AudioDevice == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice() failed %s\n", SDL_GetError());
    }

    SDL_PauseAudioDevice(AudioDevice, 0);
    
    printf("audio_len is %u\n", audio_len);
    while (audio_len > 0) {
        printf("audio_len is %u\n", audio_len);
        SDL_Delay(1000); 
    }
}

static void quit()
{
    SDL_FreeWAV(wav_buffer);
    SDL_CloseAudioDevice(AudioDevice);
    SDL_Quit();
}

int32_t main(int32_t argc, char *argv[])
{
    init();

    quit();
}
