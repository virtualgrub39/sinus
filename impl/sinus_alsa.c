#include <sinus.h>

#include <alloca.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>

#define __timeval_defined 1
#include <sys/time.h>

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

static uint64_t
now_us (void)
{
    struct timeval currentTime;
    gettimeofday (&currentTime, NULL);
    return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

struct SinusContext
{
    snd_pcm_t *pcm;
    bool running;
    SinusSettings settings;
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

    err = snd_pcm_hw_params_set_channels (pcm, hw_params, ss->channels);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_hw_params (pcm, hw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sw_params_alloca (&sw_params);

    err = snd_pcm_sw_params_current (pcm, sw_params);
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

    err = snd_pcm_sw_params (pcm, sw_params);
    if (err < 0)
    {
        TODO ("real return values");
        return -1;
    }

    err = snd_pcm_prepare (pcm);
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
        free (sc);
        fprintf (stderr, "Could not initialize ALSA (no device works)\n");
        TODO ("real return values");
        return -1;
    }

    sc->running = false;
    sc->settings
        = (SinusSettings){ .buffer_frames = _ss->buffer_frames,
                           .hint_min_write_frames = _ss->hint_min_write_frames,
                           .hint_update_us = _ss->hint_min_write_frames,
                           .channels = _ss->channels,
                           .fmt = _ss->fmt,
                           .interleaved = _ss->interleaved,
                           .sample_rate = _ss->sample_rate };

    *_sc = sc;
    return 0;
}

void
sinus_context_deinit (SinusContext *sc)
{
    runtime_assert (sc != NULL);

    sc->running = false;

    if (sc->pcm != NULL)
    {
        // snd_pcm_drain (sc->pcm);
        snd_pcm_close (sc->pcm);
        sc->pcm = NULL;
    }

    free (sc);
}

/* Start processing frames */
int
sinus_control_start (SinusContext *sc)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    if (sc->running)
        return 0;

    snd_pcm_state_t st = snd_pcm_state (sc->pcm);
    if (st == SND_PCM_STATE_SUSPENDED)
    {
        int r = snd_pcm_resume (sc->pcm);
        if (r < 0)
        {
            snd_pcm_prepare (sc->pcm);
        }
    }

    int err = snd_pcm_pause (sc->pcm, 0);
    if (err < 0)
    {
        if (err == -ENOSYS || err == -EOPNOTSUPP)
        {
            err = snd_pcm_start (sc->pcm);
            if (err < 0)
            {
                snd_pcm_prepare (sc->pcm);
                err = snd_pcm_start (sc->pcm);
            }
        }
        else
        {
            snd_pcm_prepare (sc->pcm);
            err = snd_pcm_start (sc->pcm);
        }
    }

    if (err == 0)
    {
        sc->running = true;
        return 0;
    }

    return -1;
}

/* Stop processing frames */
int
sinus_control_pause (SinusContext *sc)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    if (!sc->running)
        return 0;

    int err = snd_pcm_pause (sc->pcm, 1);
    if (err < 0)
    {
        if (err == -ENOSYS || err == -EOPNOTSUPP)
        {
            /* Pause not supported */
            TODO ("Keep internal pause flag or something :3");
            return -1;
        }
        return -1;
    }

    sc->running = false;

    return 0;
}

/* Stop processing frames & Reset internal state */
int
sinus_control_stop (SinusContext *sc)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    if (!sc->running)
        return 0;

    int err = snd_pcm_drop (sc->pcm);
    if (err < 0)
    {
        snd_pcm_prepare (sc->pcm);
        return -1;
    }

    err = snd_pcm_prepare (sc->pcm);
    if (err < 0)
        return -1;

    sc->running = false;

    return 0;
}

sinus_ssize_t
sinus_frames_write (SinusContext *sc, const void *frames, uint32_t nframes)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    if (!sc->running)
        return 0;

    snd_pcm_state_t st = snd_pcm_state (sc->pcm);
    if (st != SND_PCM_STATE_RUNNING)
    {
        return 0;
    }

    snd_pcm_sframes_t ret = snd_pcm_writei (sc->pcm, frames, nframes);
    if (ret >= 0)
        return ret;

    if (ret == -EPIPE)
    {
        snd_pcm_prepare (sc->pcm);
        return 0;
    }
    if (ret == -ESTRPIPE)
    {
        while ((ret = snd_pcm_resume (sc->pcm)) == -EAGAIN)
            sleep (1);
        if (ret < 0)
            snd_pcm_prepare (sc->pcm);
        return 0;
    }

    snd_pcm_recover (sc->pcm, ret, 1);
    return 0;
}

sinus_ssize_t
sinus_frames_write_timed (SinusContext *sc, const void *frames,
                          uint32_t nframes, uint32_t timeout_us)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    if (nframes == 0 || !frames)
        return 0;

    if (snd_pcm_state (sc->pcm) != SND_PCM_STATE_RUNNING
        || sc->running == false)
        return 0;

    uint64_t deadline = now_us () + (uint64_t)timeout_us;
    uint32_t frames_left = nframes;
    sinus_ssize_t total_written = 0;

    while (frames_left > 0)
    {
        uint64_t now = now_us ();
        if (now >= deadline)
            break; /* timeout expired */
        uint64_t rem_us = deadline - now;
        long rem_ms = (long)((rem_us + 999) / 1000); /* ceil to ms */

        snd_pcm_sframes_t avail = snd_pcm_avail_update (sc->pcm);
        if (avail < 0)
        {
            if (avail == -EPIPE)
            {
                /* underrun: prepare and continue (buffer is empty) */
                snd_pcm_prepare (sc->pcm);
                continue;
            }
            if (avail == -ESTRPIPE)
            {
                /* suspended: try to resume, then prepare if needed */
                int r = snd_pcm_resume (sc->pcm);
                if (r == -EAGAIN)
                {
                    int w = snd_pcm_wait (sc->pcm, rem_ms);
                    if (w <= 0)
                        break;
                    continue;
                }
                if (r < 0)
                {
                    snd_pcm_prepare (sc->pcm);
                    continue;
                }
                continue;
            }

            int rec = snd_pcm_recover (sc->pcm, (int)avail, 1);
            if (rec < 0)
                return -rec;
            continue;
        }

        if (avail == 0)
        {
            int w = snd_pcm_wait (sc->pcm, rem_ms);
            if (w == 0)
                break;
            if (w < 0)
            {
                snd_pcm_sframes_t rec = snd_pcm_recover (sc->pcm, w, 1);
                if (rec < 0)
                    return -rec;
                continue;
            }
            continue;
        }

        uint32_t to_write = (uint32_t)avail;
        if (to_write > frames_left)
            to_write = frames_left;

        char* ptr = (char*)frames;

        snd_pcm_sframes_t wr = snd_pcm_writei (sc->pcm, ptr, to_write);
        if (wr >= 0)
        {
            /* wrote wr frames */
            // ptr += (size_t)wr * sc->settings.hint_min_write_frames;
            ptr += (size_t)wr * sinus_format_to_size(sc->settings.fmt);
            frames_left -= (uint32_t)wr;
            total_written += wr;
            continue;
        }

        if (wr == -EPIPE)
        {
            // underrun
            snd_pcm_prepare (sc->pcm);
            continue;
        }
        if (wr == -ESTRPIPE)
        {
            /* suspended: attempt to resume; if can't, prepare; then continue */
            int r = snd_pcm_resume (sc->pcm);
            if (r == -EAGAIN)
            {
                int w = snd_pcm_wait (sc->pcm, rem_ms);
                if (w <= 0)
                    break;
                continue;
            }
            if (r < 0)
                snd_pcm_prepare (sc->pcm);
            continue;
        }
        if (wr == -EAGAIN)
        {
            /* non-blocking device: nothing to write now, wait and retry */
            int w = snd_pcm_wait (sc->pcm, rem_ms);
            if (w <= 0)
                break;
            continue;
        }

        /* For other errors, try recover; snd_pcm_recover returns >=0 on
         * success, negative on fatal error */
        {
            int rec = snd_pcm_recover (sc->pcm, (int)wr, 1);
            if (rec < 0)
                return -rec;
            continue;
        }
    }

    return total_written;
}

int
sinus_control_drain (SinusContext *sc)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    int err;

    snd_pcm_state_t st = snd_pcm_state (sc->pcm);

    if (!sc->running || st == SND_PCM_STATE_PAUSED) {
        return -1;
    }

    /* Now call drain() and handle transient ALSA errors (EINTR, ESTRPIPE,
     * EPIPE) */
    for (;;)
    {
        err = snd_pcm_drain (sc->pcm);
        if (err == 0)
            break; /* success: all queued frames played */
        if (err == -EINTR)
            continue; /* interrupted by signal — retry */

        if (err == -ESTRPIPE)
        {
            /* suspended: try to resume then retry drain */
            int r;
            while ((r = snd_pcm_resume (sc->pcm)) == -EAGAIN)
                sleep (1);
            if (r < 0)
                snd_pcm_prepare (sc->pcm);
            continue;
        }

        if (err == -EPIPE)
        {
            /* underrun/xrun while draining — reset device and return error */
            snd_pcm_prepare (sc->pcm);
            return err;
        }

        /* try recover for other transient errors */
        {
            int r = snd_pcm_recover (sc->pcm, err, 1);
            if (r == 0)
            {
                /* recovered — retry drain */
                continue;
            }
            else
            {
                /* unrecoverable */
                return r;
            }
        }
    }

    err = snd_pcm_pause (sc->pcm, 1);
    if (err < 0)
    {
        if (err == -ENOSYS || err == -EOPNOTSUPP)
        {
            /* pause not supported: keep prepared state */
            snd_pcm_prepare (sc->pcm);
        }
        else if (err == -ESTRPIPE)
        {
            /* suspended: try prepare */
            snd_pcm_prepare (sc->pcm);
        }
        else
        {
            /* any other error: ensure device is prepared */
            snd_pcm_prepare (sc->pcm);
        }
    }

    return 0;
}

sinus_ssize_t
sinus_frames_get_n_frames_buffered (SinusContext *sc)
{
    runtime_assert (sc != NULL);
    runtime_assert (sc->pcm != NULL);

    snd_pcm_sframes_t avail = 0;
    snd_pcm_sframes_t delay = 0;

    /* Try the combined helper (avail + delay in sync) */
    int err = snd_pcm_avail_delay (sc->pcm, &avail, &delay);
    if (err == 0)
    {
        /* delay is the number of frames queued to be played */
        if (delay < 0) /* defensive: negative delay can happen on suspend */
            return 0;
        return (sinus_ssize_t)delay;
    }

    /* If avail_delay isn't supported or failed, try status() + get_delay() */
    if (err == -ENOSYS || err == -EOPNOTSUPP || err < 0)
    {
        snd_pcm_status_t *status;
        snd_pcm_status_alloca (&status);
        err = snd_pcm_status (sc->pcm, status);
        if (err == 0)
        {
            snd_pcm_sframes_t sdelay = snd_pcm_status_get_delay (status);
            if (sdelay < 0)
                return 0;
            return (sinus_ssize_t)sdelay;
        }
    }

    /* Handle common ALSA error codes conservatively */
    if (err == -EPIPE)
    {
        /* underrun — buffer is effectively empty after prepare */
        snd_pcm_prepare (sc->pcm);
        return 0;
    }

    if (err == -ESTRPIPE)
    {
        /* suspended — try to resume; but conservatively return 0 */
        int r = snd_pcm_resume (sc->pcm);
        if (r < 0)
            snd_pcm_prepare (sc->pcm);
        return 0;
    }

    /* Other error: try recover once, then query again */
    snd_pcm_sframes_t rec = snd_pcm_recover (sc->pcm, err, 1);
    if (rec >= 0)
    {
        /* try avail_delay again */
        if (snd_pcm_avail_delay (sc->pcm, &avail, &delay) == 0)
        {
            if (delay < 0)
                return 0;
            return (sinus_ssize_t)delay;
        }
    }

    /* If we reach here, return a negative value so caller can know it failed */
    return (sinus_ssize_t)err;
}

sinus_ssize_t
sinus_frames_get_n_frames_free (SinusContext *sc)
{
    snd_pcm_sframes_t nframes = snd_pcm_avail_update (sc->pcm);
    // TODO ("Fancy error handling");
    return nframes;
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
