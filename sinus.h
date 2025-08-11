#ifndef _SINUS_H
#define _SINUS_H

#include <stdint.h>

typedef int64_t sinus_ssize_t;

typedef struct SinusContext SinusContext;

typedef enum sinus_format_e
{
    SINUS_FORMAT_UNKNOWN,
    SINUS_FORMAT_S8,
    SINUS_FORMAT_U8,
    SINUS_FORMAT_S16,
    SINUS_FORMAT_U16,
    SINUS_FORMAT_S24_U4,
    SINUS_FORMAT_U24_U4,
    SINUS_FORMAT_S24_P3,
    SINUS_FORMAT_U24_P3,
    SINUS_FORMAT_FLOAT,   // in range -1.0 - 1.0, 32 bit
    SINUS_FORMAT_FLOAT64, // in range -1.0 - 1.0, 64 bit
} SinusFormat;

typedef struct sinus_settings_s
{
    SinusFormat fmt;        // sample format
    uint32_t sample_rate;   // sample rate
    uint32_t channels;      // number of audio channels
    uint32_t interleaved;   // true: (LRLRL...); false: (LLLL...RRRR...)
    uint32_t buffer_frames; // sample buffer size (in frames)

    uint32_t hint_update_us;        // how often to write data to the backend
    uint32_t hint_min_write_frames; // minimum efficient write size
} SinusSettings;

void sinus_settings_default (SinusSettings *ss);

int sinus_context_init (SinusContext **sc, const SinusSettings *ss,
                        void *user_data);
void sinus_context_deinit (SinusContext *sc);

/* Start processing frames */
int sinus_control_start (SinusContext *sc);
/* Stop processing frames */
int sinus_control_pause (SinusContext *sc);
/* Stop processing frames & Reset internal state */
int sinus_control_stop (SinusContext *sc);

sinus_ssize_t sinus_frames_write (SinusContext *sc, const void *frames,
                                  uint32_t nframes);
sinus_ssize_t sinus_frames_write_timed (SinusContext *sc, const void *frames,
                                        uint32_t nframes, uint32_t timeout_us);
sinus_ssize_t sinus_frames_get_n_frames_buffered (SinusContext *sc);
sinus_ssize_t sinus_frames_get_n_frames_free (SinusContext *sc);

uint32_t sinus_info_get_sample_rate (SinusContext *sc);
uint32_t sinus_info_get_channels (SinusContext *sc);
SinusFormat sinus_info_get_format (SinusContext *sc);

/* MUTUALLY EXCLUSIVE WITH sinus_frames_write* FUNCTIONS !!!*/
typedef sinus_ssize_t (*SinusFillCallback) (void *frames,
                                            uint32_t frames_needed);
sinus_ssize_t sinus_frames_fill_callback_set (SinusContext *sc,
                                              SinusFillCallback cb);

#endif
