#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // usleep

#include "sinus.h"

static volatile int g_running = 1;
static void
sigint_handler (int sig)
{
    (void)sig;
    g_running = 0;
}

static uint32_t
read_u32_le (FILE *f)
{
    uint8_t b[4];
    if (fread (b, 1, 4, f) != 4)
        return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16)
           | ((uint32_t)b[3] << 24);
}

static uint16_t
read_u16_le (FILE *f)
{
    uint8_t b[2];
    if (fread (b, 1, 2, f) != 2)
        return 0;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

static uint32_t
wav_find_chunk (FILE *f, const char id[4])
{
    if (fseek (f, 12, SEEK_SET) != 0)
        return 0; // skip RIFF header
    while (!feof (f))
    {
        char chunkid[5] = { 0 };
        if (fread (chunkid, 1, 4, f) != 4)
            break;
        uint32_t chunk_size = read_u32_le (f);
        if (strncmp (chunkid, id, 4) == 0)
        {
            return chunk_size;
        }
        // skip chunk (and pad byte if odd)
        long skip = (long)chunk_size;
        if (fseek (f, skip + (chunk_size & 1), SEEK_CUR) != 0)
            break;
    }
    return 0;
}

static int
wav_read_fmt (FILE *f, uint16_t *audio_format, uint16_t *channels,
              uint32_t *sample_rate, uint16_t *bits_per_sample)
{
    uint32_t fmt_size = wav_find_chunk (f, "fmt ");
    if (!fmt_size)
        return 0;
    long pos = ftell (f);
    if (pos < 0)
        return 0;

    *audio_format = read_u16_le (f);
    *channels = read_u16_le (f);
    *sample_rate = read_u32_le (f);
    (void)read_u32_le (f); // byte rate
    (void)read_u16_le (f); // block align
    *bits_per_sample = read_u16_le (f);

    // if fmt_size > 16 there may be extra bytes; skip remainder
    long after = pos + (long)fmt_size;
    fseek (f, after, SEEK_SET);
    return 1;
}

static int
wav_find_data_chunk (FILE *f, long *out_offset, uint32_t *out_size)
{
    if (fseek (f, 12, SEEK_SET) != 0)
        return 0;
    while (!feof (f))
    {
        char chunkid[5] = { 0 };
        if (fread (chunkid, 1, 4, f) != 4)
            break;
        uint32_t chunk_size = read_u32_le (f);
        long chunk_data_pos = ftell (f);
        if (strncmp (chunkid, "data", 4) == 0)
        {
            *out_offset = chunk_data_pos;
            *out_size = chunk_size;
            return 1;
        }
        // skip chunk (and pad byte if odd)
        long skip = (long)chunk_size;
        if (fseek (f, skip + (chunk_size & 1), SEEK_CUR) != 0)
            break;
    }
    return 0;
}

static inline int16_t
float_to_s16_clamp (float v)
{
    if (v >= 1.0f)
        return INT16_MAX;
    if (v <= -1.0f)
        return INT16_MIN;
    return (int16_t)(v * 32767.0f);
}

int
main (int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s file.wav\n", argv[0]);
        return 1;
    }

    signal (SIGINT, sigint_handler);

    const char *fn = argv[1];
    FILE *f = fopen (fn, "rb");
    if (!f)
    {
        perror ("fopen");
        return 2;
    }

    // Validate RIFF/WAVE header
    char riff[5] = { 0 };
    if (fread (riff, 1, 4, f) != 4 || strncmp (riff, "RIFF", 4) != 0)
    {
        fprintf (stderr, "Not a RIFF file\n");
        fclose (f);
        return 3;
    }
    (void)read_u32_le (f); // file size
    char wave[5] = { 0 };
    if (fread (wave, 1, 4, f) != 4 || strncmp (wave, "WAVE", 4) != 0)
    {
        fprintf (stderr, "Not a WAVE file\n");
        fclose (f);
        return 4;
    }

    uint16_t audio_format = 0, channels = 0, bits_per_sample = 0;
    uint32_t sample_rate = 0;
    if (!wav_read_fmt (f, &audio_format, &channels, &sample_rate,
                       &bits_per_sample))
    {
        fprintf (stderr, "Failed to read fmt chunk\n");
        fclose (f);
        return 5;
    }
    long data_offset = 0;
    uint32_t data_size = 0;
    if (!wav_find_data_chunk (f, &data_offset, &data_size))
    {
        fprintf (stderr, "No data chunk found\n");
        fclose (f);
        return 6;
    }

    printf ("File: %s\n", fn);
    printf (
        "Format: %s, channels: %u, sample_rate: %u, bits: %u, data_size: %u\n",
        (audio_format == 1   ? "PCM"
         : audio_format == 3 ? "FLOAT"
                             : "OTHER"),
        (unsigned)channels, (unsigned)sample_rate, (unsigned)bits_per_sample,
        (unsigned)data_size);

    SinusSettings ss;
    sinus_settings_default (&ss);
    ss.fmt = SINUS_FORMAT_S16;
    ss.sample_rate = sample_rate;
    ss.channels = channels;
    ss.interleaved = 1;
    ss.buffer_frames = 4096;
    ss.hint_update_us = 20000; // 20ms
    ss.hint_min_write_frames = 64;

    SinusContext *sc = NULL;
    if (sinus_context_init (&sc, &ss, NULL) != 0)
    {
        fprintf (stderr, "sinus_context_init failed\n");
        fclose (f);
        return 7;
    }

    if (sinus_control_start (sc) != 0)
    {
        fprintf (stderr, "sinus_control_start failed\n");
        sinus_context_deinit (sc);
        fclose (f);
        return 8;
    }

    // Move to data chunk start
    if (fseek (f, data_offset, SEEK_SET) != 0)
    {
        perror ("fseek data");
        sinus_control_stop (sc);
        sinus_context_deinit (sc);
        fclose (f);
        return 9;
    }

    // Buffer sizes
    const uint32_t out_channels = ss.channels;
    const uint32_t read_frame_block = 1024; // frames per read
    size_t in_frame_size_bytes;             // bytes per frame in input file

    if (audio_format == 1 && bits_per_sample == 16)
    {
        in_frame_size_bytes = (size_t)channels * 2;
    }
    else if (audio_format == 3 && bits_per_sample == 32)
    {
        // float32
        in_frame_size_bytes = (size_t)channels * 4;
    }
    else
    {
        fprintf (stderr, "Unsupported WAV format: audio_format=%u bits=%u\n",
                 (unsigned)audio_format, (unsigned)bits_per_sample);
        sinus_control_stop (sc);
        sinus_context_deinit (sc);
        fclose (f);
        return 10;
    }

    // allocate buffers
    uint32_t out_frame_bytes = out_channels * sizeof (int16_t);
    size_t in_buf_size = in_frame_size_bytes * read_frame_block;
    void *in_buf = malloc (in_buf_size);
    if (!in_buf)
    {
        perror ("malloc");
        goto cleanup;
    }
    // interleaved output buffer (int16_t)
    int16_t *out_buf = malloc (out_frame_bytes * read_frame_block);
    if (!out_buf)
    {
        perror ("malloc");
        goto cleanup;
    }

    uint32_t frames_left_in_data = data_size / (uint32_t)in_frame_size_bytes;
    uint64_t total_frames = frames_left_in_data;
    uint64_t played_frames = 0;

    while (g_running && frames_left_in_data > 0)
    {
        uint32_t to_read = read_frame_block;
        if (to_read > frames_left_in_data)
            to_read = frames_left_in_data;
        size_t r = fread (in_buf, in_frame_size_bytes, to_read, f);
        if (r != to_read)
        {
            fprintf (stderr, "Short read: expected %u got %zu\n", to_read, r);
            to_read = (uint32_t)r;
        }
        if (to_read == 0)
            break;

        // Convert input to output S16 interleaved
        if (audio_format == 1 && bits_per_sample == 16)
        {
            // Input is native little-endian S16 interleaved (commonly)
            // But WAV is interleaved as well -> copy + channel adjustments
            int16_t *in16 = (int16_t *)in_buf;
            for (uint32_t fidx = 0; fidx < to_read; ++fidx)
            {
                for (uint32_t ch = 0; ch < out_channels; ++ch)
                {
                    int16_t val = 0;
                    if (ch < channels)
                    {
                        val = in16[fidx * channels + ch];
                    }
                    else
                    {
                        // if input has fewer channels, duplicate last channel
                        // (or channel 0)
                        val = in16[fidx * channels + (channels - 1)];
                    }
                    out_buf[fidx * out_channels + ch] = val;
                }
            }
        }
        else if (audio_format == 3 && bits_per_sample == 32)
        {
            // float32 input
            float *in32 = (float *)in_buf;
            for (uint32_t fidx = 0; fidx < to_read; ++fidx)
            {
                for (uint32_t ch = 0; ch < out_channels; ++ch)
                {
                    float v = 0.0f;
                    if (ch < channels)
                    {
                        v = in32[fidx * channels + ch];
                    }
                    else
                    {
                        v = in32[fidx * channels + (channels - 1)];
                    }
                    out_buf[fidx * out_channels + ch] = float_to_s16_clamp (v);
                }
            }
        }

        // Now write to sinus in chunks depending on available free frames
        uint32_t frames_written_from_block = 0;
        while (g_running && frames_written_from_block < to_read)
        {
            sinus_ssize_t freef = sinus_frames_get_n_frames_free (sc);
            if (freef < 0)
            {
                fprintf (stderr, "sinus_frames_get_n_frames_free failed\n");
                g_running = 0;
                break;
            }
            if (freef == 0)
            {
                // backend buffer full - wait a bit
                usleep (5000); // 5ms
                continue;
            }

            uint32_t can_write = (uint32_t)freef;
            uint32_t remaining = to_read - frames_written_from_block;
            if (can_write > remaining)
                can_write = remaining;

            // compute pointer and size
            void *write_ptr
                = &out_buf[(size_t)frames_written_from_block * out_channels];
            sinus_ssize_t wrote = sinus_frames_write_timed (
                sc, write_ptr, can_write, 200000 /*200ms timeout*/);
            if (wrote < 0)
            {
                fprintf (stderr, "sinus_frames_write_timed error: %zd\n",
                         (ssize_t)wrote);
                g_running = 0;
                break;
            }
            if (wrote == 0)
            {
                // timed out; try again
                usleep (2000);
                continue;
            }
            frames_written_from_block += (uint32_t)wrote;
        }

        played_frames += to_read;
        frames_left_in_data -= to_read;

        // print progress occasionally
        static uint64_t last_print = 0;
        if (played_frames - last_print >= (uint64_t)sample_rate)
        { // every second
            last_print = played_frames;
            double pct
                = total_frames ? (played_frames * 100.0 / total_frames) : 0.0;
            sinus_ssize_t buf_frames = sinus_frames_get_n_frames_buffered (sc);
            printf ("\rPlayed: %llu/%llu frames (%.1f%%), buffered: %zd   \n",
                    (unsigned long long)played_frames,
                    (unsigned long long)total_frames, pct, (ssize_t)buf_frames);
            fflush (stdout);
        }
    }

    // EOF or stopped: wait for backend buffer to drain
    printf ("\nFile playback finished or stopped. Waiting for buffered frames "
            "to drain...\n");
    sinus_control_drain(sc);

    printf ("Stopping audio...\n");
    sinus_control_stop (sc);

cleanup:
    if (in_buf)
        free (in_buf);
    if (out_buf)
        free (out_buf);

    if (sc)
        sinus_context_deinit (sc);
    if (f)
        fclose (f);

    printf ("Done.\n");
    return 0;
}
