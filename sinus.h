#ifndef _SINUS_H
#define _SINUS_H

#include <stdint.h>

#ifndef SINUSDEF
#define SINUSDEF
#endif

#ifndef SINUS_SSIZE_T_DEFINED
// TODO: move stdint inclusion here
typedef int64_t sinus_ssize_t;
#endif

// TODO: replace all uint32_t's with some re-definable type

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

static const sinus_ssize_t sinus_format_sizes_bytes[] = {
    [SINUS_FORMAT_UNKNOWN] = 0, [SINUS_FORMAT_S8] = 1,
    [SINUS_FORMAT_U8] = 1,      [SINUS_FORMAT_S16] = 2,
    [SINUS_FORMAT_U16] = 2,     [SINUS_FORMAT_S24_U4] = 4,
    [SINUS_FORMAT_U24_U4] = 4,  [SINUS_FORMAT_S24_P3] = 3,
    [SINUS_FORMAT_U24_P3] = 3,  [SINUS_FORMAT_FLOAT] = 4,
    [SINUS_FORMAT_FLOAT64] = 8,
};

#define sinus_format_to_size(fmt) sinus_format_sizes_bytes[fmt]

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

SINUSDEF void sinus_settings_default (SinusSettings *ss);

SINUSDEF int sinus_context_init (SinusContext **sc, const SinusSettings *ss,
                                 void *user_data);
SINUSDEF void sinus_context_deinit (SinusContext *sc);

/* Start processing frames */
SINUSDEF int sinus_control_start (SinusContext *sc);
/* Stop processing frames */
SINUSDEF int sinus_control_pause (SinusContext *sc);
/* Stop processing frames & Reset internal state */
SINUSDEF int sinus_control_stop (SinusContext *sc);
/* Process all queued frames and pause */
SINUSDEF int sinus_control_drain (SinusContext *sc);

SINUSDEF sinus_ssize_t sinus_frames_write (SinusContext *sc, const void *frames,
                                           uint32_t nframes);
SINUSDEF sinus_ssize_t sinus_frames_write_timed (SinusContext *sc,
                                                 const void *frames,
                                                 uint32_t nframes,
                                                 uint32_t timeout_us);
SINUSDEF sinus_ssize_t sinus_frames_get_n_frames_buffered (SinusContext *sc);
SINUSDEF sinus_ssize_t sinus_frames_get_n_frames_free (SinusContext *sc);

SINUSDEF uint32_t sinus_info_get_sample_rate (SinusContext *sc);
SINUSDEF uint32_t sinus_info_get_channels (SinusContext *sc);
SINUSDEF SinusFormat sinus_info_get_format (SinusContext *sc);

/* MUTUALLY EXCLUSIVE WITH sinus_frames_write* FUNCTIONS !!!*/
typedef sinus_ssize_t (*SinusFillCallback) (void *frames,
                                            uint32_t frames_needed);
SINUSDEF sinus_ssize_t sinus_frames_fill_callback_set (SinusContext *sc,
                                                       SinusFillCallback cb);

#endif
