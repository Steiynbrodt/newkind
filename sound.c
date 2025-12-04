/*
 * Elite - The New Kind.
 *
 * Allegro 5 sound backend.
 *
 * Original: Allegro 4 version by C.J.Pinder, 1999-2001.
 * Ported to Allegro 5.
 */

#include <stdlib.h>
#include <string.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "sound.h"
#include "alg_data.h"   /* still included in case other code needs it, but MIDI is stubbed */

/* Number of sound effects we manage */
#define NUM_SAMPLES 14

static int sound_on = 0;

struct sound_sample
{
    ALLEGRO_SAMPLE *sample;
    char filename[256];
    int  runtime;   /* number of ticks the sound is “busy” */
    int  timeleft;  /* countdown, used to avoid spamming the sound */
};

static struct sound_sample sample_list[NUM_SAMPLES] =
{
    { NULL, "launch.wav",    32, 0 },
    { NULL, "crash.wav",      7, 0 },
    { NULL, "dock.wav",      36, 0 },
    { NULL, "gameover.wav",  24, 0 },
    { NULL, "pulse.wav",      4, 0 },
    { NULL, "hitem.wav",      4, 0 },
    { NULL, "explode.wav",   23, 0 },
    { NULL, "ecm.wav",       23, 0 },
    { NULL, "missile.wav",   25, 0 },
    { NULL, "hyper.wav",     37, 0 },
    { NULL, "incom1.wav",     4, 0 },
    { NULL, "incom2.wav",     5, 0 },
    { NULL, "beep.wav",       2, 0 },
    { NULL, "boop.wav",       7, 0 },
};


/*
 * Initialise Allegro 5 audio and load all samples.
 * Assumes al_init() has already been called somewhere in your program.
 */
void snd_sound_startup(void)
{
    int i;

    sound_on = 1;

    if (!al_is_system_installed())
    {
        /* al_init() must be called before this module is used */
        sound_on = 0;
        return;
    }

    if (!al_is_audio_installed())
    {
        if (!al_install_audio())
        {
            sound_on = 0;
            return;
        }
    }

    /* Enable codecs for WAV/OGG/MP3 etc. */
    if (!al_is_acodec_addon_initialized())
        al_init_acodec_addon();

    /* Reserve a few mixing slots for sample playback. */
    if (!al_is_audio_installed() || !al_reserve_samples(NUM_SAMPLES + 4))
    {
        sound_on = 0;
        return;
    }

    /* Load the sound samples... */
    for (i = 0; i < NUM_SAMPLES; i++)
    {
        if (sample_list[i].sample)
        {
            al_destroy_sample(sample_list[i].sample);
            sample_list[i].sample = NULL;
        }

        sample_list[i].sample = al_load_sample(sample_list[i].filename);

        /* If loading fails, we just leave sample == NULL and ignore it later. */
    }
}


/*
 * Shutdown audio and free all loaded samples.
 */
void snd_sound_shutdown(void)
{
    int i;

    if (!sound_on)
        return;

    for (i = 0; i < NUM_SAMPLES; i++)
    {
        if (sample_list[i].sample != NULL)
        {
            al_destroy_sample(sample_list[i].sample);
            sample_list[i].sample = NULL;
        }
    }

    /* We could call al_uninstall_audio() here, but if the rest of the
       program might still use audio, better leave it installed. */
}


/*
 * Play one of the sound effect samples.
 */
void snd_play_sample(int sample_no)
{
    if (!sound_on)
        return;

    if (sample_no < 0 || sample_no >= NUM_SAMPLES)
        return;

    if (sample_list[sample_no].sample == NULL)
        return;

    if (sample_list[sample_no].timeleft != 0)
        return;

    sample_list[sample_no].timeleft = sample_list[sample_no].runtime;

    /* volume: 1.0, pan: 0.0 (center), speed: 1.0, play once */
    al_play_sample(sample_list[sample_no].sample,
                   1.0f, 0.0f, 1.0f,
                   ALLEGRO_PLAYMODE_ONCE, NULL);
}


/*
 * Called once per game tick to update the "busy time" counters.
 */
void snd_update_sound(void)
{
    int i;

    if (!sound_on)
        return;

    for (i = 0; i < NUM_SAMPLES; i++)
    {
        if (sample_list[i].timeleft > 0)
            sample_list[i].timeleft--;
    }
}


/*
 * MIDI playback – Allegro 5 no longer has built-in MIDI support.
 * You can replace this with streaming OGG/MP3 music if you want.
 * For now, we stub these out so the rest of the code compiles.
 */

void snd_play_midi(int midi_no, int repeat)
{
    (void)midi_no;
    (void)repeat;

    /* TODO: Implement using ALLEGRO_AUDIO_STREAM and real music files,
       e.g. load "theme.ogg" / "danube.ogg" based on midi_no and loop if repeat. */
}

void snd_stop_midi(void)
{
    /* TODO: stop any currently playing music stream, if you implement one. */
}
