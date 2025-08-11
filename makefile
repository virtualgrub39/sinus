
all: libsinus-alsa.a test

LDFLAGS =
CFLAGS  = -I./ -Wall -Wextra -pedantic -Werror -std=c99

ALSA_LDFLAGS = $(LDFLAGS) -lasound
ALSA_CFLAGS = $(CFLAGS)

libsinus-alsa.a: libsinus-alsa.o
# 	gcc libsinus-alsa.o $(ALSA_LDFLAGS) -o libsinus-alsa.a
	ar rcs libsinus-alsa.a libsinus-alsa.o

libsinus-alsa.o: sinus.h impl/sinus_alsa.c
	gcc -c impl/sinus_alsa.c -o libsinus-alsa.o $(ALSA_CFLAGS)

test: test.c libsinus-alsa.a sinus.h
	gcc test.c libsinus-alsa.a -o test -lasound
