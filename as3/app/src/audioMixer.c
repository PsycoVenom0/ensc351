#include "audioMixer.h"
#include "periodTimer.h" 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <limits.h>
#include <alloca.h>

static snd_pcm_t *handle;

#define DEFAULT_VOLUME 80
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 1
#define SAMPLE_SIZE (sizeof(short)) 

static unsigned long playbackBufferSize = 0;
static short *playbackBuffer = NULL;

#define MAX_SOUND_BITES 30
typedef struct {
    wavedata_t *pSound;
    int location;
} playbackSound_t;
static playbackSound_t soundBites[MAX_SOUND_BITES];

void* playbackThread(void* arg);
static _Bool stopping = false;
static pthread_t playbackThreadId;
static pthread_mutex_t audioMutex = PTHREAD_MUTEX_INITIALIZER;
static int volume = 0;

void AudioMixer_init(void)
{
    AudioMixer_setVolume(DEFAULT_VOLUME);
    for(int i = 0; i < MAX_SOUND_BITES; i++) {
        soundBites[i].pSound = NULL;
        soundBites[i].location = 0;
    }

    int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_set_params(handle,
            SND_PCM_FORMAT_S16_LE,
            SND_PCM_ACCESS_RW_INTERLEAVED,
            NUM_CHANNELS,
            SAMPLE_RATE,
            1,
            50000);
    if (err < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    unsigned long unusedBufferSize = 0;
    snd_pcm_get_params(handle, &unusedBufferSize, &playbackBufferSize);
    playbackBuffer = malloc(playbackBufferSize * sizeof(*playbackBuffer));

    pthread_create(&playbackThreadId, NULL, playbackThread, NULL);
}

void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound)
{
    assert(pSound);
    const int PCM_DATA_OFFSET = 44;
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
        exit(EXIT_FAILURE);
    }
    fseek(file, 0, SEEK_END);
    int sizeInBytes = ftell(file) - PCM_DATA_OFFSET;
    pSound->numSamples = sizeInBytes / SAMPLE_SIZE;
    fseek(file, PCM_DATA_OFFSET, SEEK_SET);
    pSound->pData = malloc(sizeInBytes);
    if (pSound->pData == 0) {
        fprintf(stderr, "ERROR: Unable to allocate %d bytes.\n", sizeInBytes);
        exit(EXIT_FAILURE);
    }
    int samplesRead = fread(pSound->pData, SAMPLE_SIZE, pSound->numSamples, file);
    if (samplesRead != pSound->numSamples) {
        fprintf(stderr, "ERROR: Unable to read samples.\n");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

void AudioMixer_freeWaveFileData(wavedata_t *pSound)
{
    pSound->numSamples = 0;
    free(pSound->pData);
    pSound->pData = NULL;
}

void AudioMixer_queueSound(wavedata_t *pSound)
{
    if (pSound == NULL || pSound->numSamples <= 0) return;

    pthread_mutex_lock(&audioMutex);
    int foundSlot = 0;
    for (int i = 0; i < MAX_SOUND_BITES; i++) {
        if (soundBites[i].pSound == NULL) {
            soundBites[i].pSound = pSound;
            soundBites[i].location = 0;
            foundSlot = 1;
            break;
        }
    }
    pthread_mutex_unlock(&audioMutex);

    // FIX: Use the foundSlot variable to check for errors
    if (!foundSlot) {
        fprintf(stderr, "ERROR: Audio Mixer is full (too many sounds playing)\n");
    }
}

void AudioMixer_cleanup(void)
{
    printf("Stopping audio...\n");
    stopping = true;
    pthread_join(playbackThreadId, NULL);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(playbackBuffer);
    playbackBuffer = NULL;
    printf("Done stopping audio...\n");
}

int AudioMixer_getVolume() { return volume; }

void AudioMixer_setVolume(int newVolume)
{
    if (newVolume < 0 || newVolume > AUDIOMIXER_MAX_VOLUME) return;
    volume = newVolume;
    long min, max;
    snd_mixer_t *mixerHandle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Speaker"; 

    snd_mixer_open(&mixerHandle, 0);
    snd_mixer_attach(mixerHandle, card);
    snd_mixer_selem_register(mixerHandle, NULL, NULL);
    snd_mixer_load(mixerHandle);
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(mixerHandle, sid);
    if (elem == NULL) {
        snd_mixer_selem_id_set_name(sid, "PCM");
        elem = snd_mixer_find_selem(mixerHandle, sid);
    }
    if (elem) {
        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);
    }
    snd_mixer_close(mixerHandle);
}

static void fillPlaybackBuffer(short *buff, int size)
{
    memset(buff, 0, size * sizeof(short));
    pthread_mutex_lock(&audioMutex);
    for (int i = 0; i < MAX_SOUND_BITES; i++) {
        if (soundBites[i].pSound == NULL) continue;
        wavedata_t *pWav = soundBites[i].pSound;
        int currentLoc = soundBites[i].location;
        for (int j = 0; j < size; j++) {
            if (currentLoc >= pWav->numSamples) {
                soundBites[i].pSound = NULL;
                break; 
            }
            int sample = (int)buff[j] + (int)pWav->pData[currentLoc];
            if (sample > SHRT_MAX) sample = SHRT_MAX;
            else if (sample < SHRT_MIN) sample = SHRT_MIN;
            buff[j] = (short)sample;
            currentLoc++;
        }
        if (soundBites[i].pSound != NULL) soundBites[i].location = currentLoc;
    }
    pthread_mutex_unlock(&audioMutex);
}

void* playbackThread(void* _arg)
{
    (void)_arg;
    while (!stopping) {
        fillPlaybackBuffer(playbackBuffer, playbackBufferSize);
        snd_pcm_sframes_t frames = snd_pcm_writei(handle, playbackBuffer, playbackBufferSize);
        
        Period_markEvent(PERIOD_EVENT_AUDIO_BUFFER);

        if (frames < 0) frames = snd_pcm_recover(handle, frames, 1);
        if (frames < 0) {
            fprintf(stderr, "ERROR: writei failed: %li\n", frames);
        }
    }
    return NULL;
}