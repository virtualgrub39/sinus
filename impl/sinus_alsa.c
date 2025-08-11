#include <sinus.h>

#include <alloca.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>

#define runtime_assert(condition)                                              \
    if (!(condition))                                                          \
    {                                                                          \
        fprintf (stderr, "%s:%d Runtime assertion failed: " #condition "\n",   \
                 __FILE__, __LINE__);                                          \
        abort ();                                                              \
    }

#define arrlen(arr) (sizeof (arr) / sizeof (arr[0]))

#define TODO(what)                                                             \
    do                                                                         \
    {                                                                          \
        fprintf (stderr, "%s:%u TODO: %s\n", __FILE__, __LINE__, what);        \
    } while (0)

struct SinusContext
{
    snd_pcm_t *pcm;
};

void
sinus_settings_default (SinusSettings *ss)
{
    runtime_assert (ss != NULL);

    ss->buffer_frames = 4096;
    ss->channels = 2;
    ss->fmt = SINUS_FORMAT_U24_U4;
    ss->interleaved = true;
    ss->sample_rate = 44100;
    ss->hint_min_write_frames = 1024;
    ss->hint_update_us = 24000;
}

static snd_pcm_format_t
alsa_format_from_sinus (SinusFormat fmt)
{
    switch (fmt)
    {
    case SINUS_FORMAT_UNKNOWN:
        return SND_PCM_FORMAT_UNKNOWN;
    case SINUS_FORMAT_S8:
        return SND_PCM_FORMAT_S8;
    case SINUS_FORMAT_U8:
        return SND_PCM_FORMAT_U8;
    case SINUS_FORMAT_S16:
        return SND_PCM_FORMAT_S16;
    case SINUS_FORMAT_U16:
        return SND_PCM_FORMAT_U16;
    case SINUS_FORMAT_S24_U4:
        return SND_PCM_FORMAT_S24;
    case SINUS_FORMAT_U24_U4:
        return SND_PCM_FORMAT_U24;
    case SINUS_FORMAT_S24_P3:
        return SND_PCM_FORMAT_S24_3LE;
    case SINUS_FORMAT_U24_P3:
        return SND_PCM_FORMAT_U24_3LE;
    case SINUS_FORMAT_FLOAT:
        return SND_PCM_FORMAT_FLOAT;
    case SINUS_FORMAT_FLOAT64:
        return SND_PCM_FORMAT_FLOAT64;
    }

    fprintf (stderr, "%s:%u UNREACHABLE\n", __FILE__, __LINE__);
    abort ();
}

static int
alsa_open_and_configure (SinusContext *sc, const char *devname,
                         const SinusSettings *ss)
{
    snd_pcm_t *pcm;

    int err = snd_pcm_open (&pcm, devname, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca (&hw_params);

    err = snd_pcm_hw_params_any (pcm, hw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_access_t access_type = SND_PCM_ACCESS_RW_NONINTERLEAVED;

    if (ss->interleaved)
    {
        access_type = SND_PCM_ACCESS_RW_INTERLEAVED;
    }

    err = snd_pcm_hw_params_set_access (pcm, hw_params, access_type);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_format_t formats[] = {
        alsa_format_from_sinus (ss->fmt),
        SND_PCM_FORMAT_S32_LE,
        SND_PCM_FORMAT_S24_LE,
        SND_PCM_FORMAT_S24_3LE,
        SND_PCM_FORMAT_S16_LE,
    };

    bool format_accepted = false;

    for (unsigned i = 0; i < arrlen (formats); ++i)
    {
        err = snd_pcm_hw_params_test_format (pcm, hw_params, formats[i]);
        if (err < 0)
            continue;

        err = snd_pcm_hw_params_set_format (pcm, hw_params, formats[i]);
        if (err == 0)
        {
            format_accepted = true;
            break;
        }
    }

    if (!format_accepted)
    {
        TODO ("real return values");
        return -1;
    }

    unsigned int rates[] = { ss->sample_rate, 192000, 96000, 48000, 44100 };

    bool rate_accepted = false;

    for (unsigned i = 0; i < arrlen (rates); ++i)
    {
        err = snd_pcm_hw_params_test_rate (pcm, hw_params, rates[i], 0);
        if (err < 0)
            continue;

        err = snd_pcm_hw_params_set_rate_near (pcm, hw_params, &rates[i], 0);
        if (err == 0)
        {
            rate_accepted = true;
            break;
        }
    }

    if (!rate_accepted)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_hw_params_set_channels(pcm, hw_params, ss->channels);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_hw_params(pcm, hw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sw_params_alloca(&sw_params);

    err = snd_pcm_sw_params_current(pcm, sw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_sw_params_set_avail_min (pcm, sw_params, ss->buffer_frames);
    /* TODO: are we setting this correctly? */
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_sw_params_set_start_threshold (pcm, sw_params, 0U);
    /* TODO: what does this do? */
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_sw_params(pcm, sw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_prepare(pcm);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    sc->pcm = pcm;
    return 0;
}

int
sinus_context_init (SinusContext **_sc, const SinusSettings *ss_nullable,
                    void *user_data)
{
    (void)user_data;

    runtime_assert (_sc != NULL);

    struct SinusContext *sc = malloc (sizeof (struct SinusContext));
    runtime_assert (sc != NULL);

    SinusSettings *_ss = (SinusSettings *)ss_nullable;
    if (!_ss)
    {
        _ss = alloca (sizeof (SinusSettings));
        sinus_settings_default (_ss);
    }

    const char *devnames[] = {
        "default",    "plug:default", "hw:0,0",     "plughw:0,0", "hw:1,0",
        "plughw:1,0", "pulse",        "plug:pulse", "jack",       "plug:jack",
    };

    bool configured = false;

    for (unsigned i = 0; i < arrlen (devnames); ++i)
    {
        int err = alsa_open_and_configure (sc, devnames[i], _ss);
        if (err == 0)
        {
            configured = true;
            break;
        }
    }

    if (!configured)
    {
        free(sc);
        fprintf (stderr, "Could not initialize ALSA (no device works)\n");
        TODO ("real return values");
        return -1;
    }

    *_sc = sc;
    return 0;
}

void
sinus_context_deinit (SinusContext *sc)
{
    runtime_assert (sc != NULL);

    if (sc->pcm != NULL)
    {
        snd_pcm_drain (sc->pcm);
        snd_pcm_close (sc->pcm);
        sc->pcm = NULL;
    }

    free(sc);
}

/* Start processing frames */
int
sinus_control_start (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_control_start");
    return -1;
}
/* Stop processing frames */
int
sinus_control_pause (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_control_pause");
    return -1;
}
/* Stop processing frames & Reset internal state */
int
sinus_control_stop (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_control_stop");
    return -1;
}

sinus_ssize_t
sinus_frames_write (SinusContext *sc, const void *frames, uint32_t nframes)
{
    int err = snd_pcm_wait (sc->pcm, 0);
    if (err == 0) // TIMEOUT
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_sframes_t frames_available = snd_pcm_avail_update (sc->pcm);
    if (frames_available < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_writei (sc->pcm, frames, nframes);

    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    return err;
}

sinus_ssize_t
sinus_frames_write_timed (SinusContext *sc, const void *frames,
                          uint32_t nframes, uint32_t timeout_us)
{
    (void)sc;
    (void)frames;
    (void)nframes;
    (void)timeout_us;
    TODO ("sinus_frames_write_timed");
    return -1;
}
sinus_ssize_t
sinus_frames_get_n_frames_buffered (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_frames_get_n_frames_buffered");
    return -1;
}
sinus_ssize_t
sinus_frames_get_n_frames_free (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_frames_get_n_frames_free");
    return -1;
}

uint32_t
sinus_info_get_sample_rate (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_info_get_sample_rate");
    return -1;
}
uint32_t
sinus_info_get_channels (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_info_get_channels");
    return -1;
}
SinusFormat
sinus_info_get_format (SinusContext *sc)
{
    (void)sc;
    TODO ("sinus_info_get_format");
    return -1;
}
