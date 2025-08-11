#include "sinus.h"
#include <unistd.h>

#include "square.h"

#include <stdio.h>

int
main (void)
{
    SinusContext *sc;
    sinus_context_init (&sc, NULL, NULL);

    for (uint32_t i = 0; i < 5; ++i)
    {
        sinus_ssize_t w = 0;

        while (w < SQUARE_SAMPLE_COUNT)
        {
            w += sinus_frames_write (sc, square_sample_table,
                                     SQUARE_SAMPLE_COUNT);
        }
    }

    getchar ();

    sinus_context_deinit (sc);
}
