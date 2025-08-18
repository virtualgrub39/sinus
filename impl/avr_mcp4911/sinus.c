#ifndef __AVR__
#error "This file has to be compiled with AVR C compiler"
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#define SINUSDEF static inline
typedef uint8_t sinus_ssize_t;
#define SINUS_SSIZE_T_DEFINED
#include <sinus.h>

#define PRESCALER 8U
#define TIMER_COUNTER_TOP 44U

#define SAMPLE_RATE_HZ                                                         \
    ((uint32_t)(F_CPU)                                                         \
     / ((uint32_t)(PRESCALER) * ((uint32_t)(TIMER_COUNTER_TOP) + 1U)))

#define FRAME_BUFFER_SIZE_FRAMES 8U
#define FRAME_BUFFER_SIZE_BYTES (FRAME_BUFFER_SIZE_FRAMES * 10U / 8U)

#define PIN_MOSI PB0
#define PIN_MISO PB1
#define PIN_SCK PB2
#define PIN_SLAVE_SELECT_DEFAULT PB3

#define TIMER_START TIMSK |= (1 << OCIE0A)
#define TIMER_STOP TIMSK &= ~(1 << OCIE0A)

#define USI_MODE_SPI                                                           \
    do                                                                         \
    {                                                                          \
        USICR |= (1 << USIWM0);                                                \
        USICR &= ~(1 << USIWM1);                                               \
    } while (0)
#define USI_MODE_OFF                                                           \
    do                                                                         \
    {                                                                          \
        USICR &= ~(1 << USIWM0);                                               \
        USICR &= ~(1 << USIWM1);                                               \
    } while (0)

struct SinusContext
{
    SinusSettings ss;
    uint8_t slave_select_pin;

    // frame ring buffer
    uint8_t frame_buffer[FRAME_BUFFER_SIZE_BYTES];
    uint8_t *buffer_head;
    uint8_t buffer_len;
    uint8_t *buffer_end;
};

static SinusContext _sc = { 0 };
// table for multiplying numbers by 0.8, floored
static const uint8_t mul08_table[11] = { 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8 };

SINUSDEF void
sinus_settings_default (SinusSettings *ss)
{
    ss->buffer_frames = FRAME_BUFFER_SIZE_FRAMES;
    ss->channels = 1;
    ss->hint_min_write_frames = 4;
    ss->fmt = SINUS_FORMAT_UNKNOWN; // 4U10_P5
    ss->interleaved = 0;
    ss->sample_rate = SAMPLE_RATE_HZ;
    ss->hint_update_us = 181;
}

static inline void
timer0_setup (void)
{
    TCCR0B = 0;                          // stop the timer
    TCNT0 = 0;                           // clear timer counter
    TCCR0A = (1 << WGM01);               // CTC mode
    TCCR0B = (1 << WGM01) | (1 << CS01); // prescaler = 8
    OCR0A = TIMER_COUNTER_TOP;           // 22.2 kHz
}

SINUSDEF int
sinus_context_init (SinusContext **sc, const SinusSettings *ss, void *user_data)
{
    (void)ss; // we don't accept config

    *sc = &_sc;

    sinus_settings_default (&_sc.ss);
    if (user_data)
        _sc.slave_select_pin = *(uint8_t *)user_data;
    else
        _sc.slave_select_pin = PIN_SLAVE_SELECT_DEFAULT;
    memset (_sc.frame_buffer, 0, FRAME_BUFFER_SIZE_BYTES);
    _sc.buffer_head = _sc.frame_buffer;
    _sc.buffer_len = 0;
    _sc.buffer_end = _sc.buffer_head + FRAME_BUFFER_SIZE_BYTES;

    DDRB |= (1 << PIN_MOSI) | (1 << _sc.slave_select_pin); // outputs
    PORTB |= (1 << _sc.slave_select_pin);                  // active-low

    USI_MODE_OFF;
    timer0_setup ();

    return 0;
}

SINUSDEF void
sinus_context_deinit (SinusContext *sc)
{
    (void)sc;
    USI_MODE_OFF;
    return;
}

/* Start processing frames */
SINUSDEF int
sinus_control_start (SinusContext *sc)
{
    (void)sc;
    TIMER_START;
    return 0;
}

/* Stop processing frames */
SINUSDEF int
sinus_control_pause (SinusContext *sc)
{
    (void)sc;
    TIMER_STOP;
    return 0;
}
/* Stop processing frames & Reset internal state */
SINUSDEF int
sinus_control_stop (SinusContext *sc)
{
    TIMER_STOP;
    sc->buffer_head = sc->frame_buffer;
    sc->buffer_len = 0;
    return 0;
}
/* Process all queued frames and pause */
SINUSDEF int
sinus_control_drain (SinusContext *sc)
{
    TIMER_START;
    while (sc->buffer_len > 0)
        ;
    TIMER_STOP;
    return 0;
}

SINUSDEF sinus_ssize_t
sinus_frames_write (SinusContext *sc, const void *frames, uint32_t nframes)
{
    if (sc->buffer_len == FRAME_BUFFER_SIZE_BYTES)
        return 0;

    uint8_t to_write = (nframes > sc->buffer_len) ? sc->buffer_len : nframes;
    uint8_t retval = to_write;
    const uint8_t *ptr = frames;

    sc->buffer_len += to_write;

    while (to_write > 0)
    {
        *sc->buffer_head = *ptr;
        if (sc->buffer_head == sc->buffer_end)
            sc->buffer_head = sc->frame_buffer;
        else
            sc->buffer_head += 1;
        ptr += 1;
        to_write -= 1;
    }

    return retval;
}

SINUSDEF sinus_ssize_t
sinus_frames_write_timed (SinusContext *sc, const void *frames,
                          uint32_t nframes, uint32_t timeout_us)
{
    // get start time in us

    if (sc->buffer_len == FRAME_BUFFER_SIZE_BYTES)
        return 0;

    uint8_t to_write = nframes;
    const uint8_t *ptr = frames;

    while (to_write > 0)
    {
        while (sc->buffer_len == 0)
        {
            // wait

            // if now - start > timeout_us then return nframes - to_write

            // TODO: use timer1 for this somehow???
        }

        *sc->buffer_head = *ptr;
        if (sc->buffer_head == sc->buffer_end)
            sc->buffer_head = sc->frame_buffer;
        else
            sc->buffer_head += 1;
        ptr += 1;
        to_write -= 1;
        sc->buffer_len += 1;
    }

    return nframes;
}

SINUSDEF sinus_ssize_t
sinus_frames_get_n_frames_buffered (SinusContext *sc)
{
    return mul08_table[sc->buffer_len];
}

SINUSDEF sinus_ssize_t
sinus_frames_get_n_frames_free (SinusContext *sc)
{
    uint8_t avail = (uint8_t)(FRAME_BUFFER_SIZE_BYTES - sc->buffer_len);
    return mul08_table[avail];
}

SINUSDEF uint32_t
sinus_info_get_sample_rate (SinusContext *sc)
{
    return SAMPLE_RATE_HZ;
}
SINUSDEF uint32_t
sinus_info_get_channels (SinusContext *sc)
{
    return 1;
}
SINUSDEF SinusFormat
sinus_info_get_format (SinusContext *sc)
{
    return SINUS_FORMAT_UNKNOWN;
}
