
all: libsinus-alsa.a

SINUS_PATH = ../../sinus.h

LDFLAGS =
CFLAGS  = -I../../ -Wall -Wextra -pedantic -Werror -std=c99

ALSA_LDFLAGS = $(LDFLAGS) -lasound
ALSA_CFLAGS = $(CFLAGS)

libsinus-alsa.a: libsinus-alsa.o
	ar rcs libsinus-alsa.a libsinus-alsa.o

libsinus-alsa.o: $(SINUS_PATH) sinus.c
	gcc -c sinus.c -o libsinus-alsa.o $(ALSA_CFLAGS)

clean:
	rm -rf *.o *.a

.PHONY: clean all
