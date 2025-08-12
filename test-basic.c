#include "sinus.h"

#include "square.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

int
main (void)
{
    SinusContext *sc;
    sinus_context_init (&sc, NULL, NULL);

    sinus_control_start (sc);

    for (uint32_t i = 0; i < 5; ++i)
    {
        uint32_t w = 0;

        while (w < SQUARE_SAMPLE_COUNT)
        {
            sinus_ssize_t n = sinus_frames_write (sc, square_sample_table,
                                                  SQUARE_SAMPLE_COUNT);
            if (n < 0)
            {
                break;
            }

            w += n;
        }
    }

    for (uint64_t i = 0; i < 128; ++i) {
        int waste_of_time = 1;

        for (uint64_t j = 0; j < UINT16_MAX * 16; ++j) {
            waste_of_time = j % INT32_MAX - 1;
        }

        printf(".");
        fflush(stdout);
    }

    printf("\nDone wasting time\n");

    getchar ();

    sinus_context_deinit (sc);
    return 0;
}
