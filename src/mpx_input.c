/*
    PiFmAdv - Advanced FM transmitter for the Raspberry Pi
    Copyright (C) 2017 Miegl

    See https://github.com/Miegl/PiFmAdv
*/

#include <sndfile.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mpx_input.h"

#define MPX_INPUT_BUF_LEN 4096

int mpx_input_samplerate;

static SNDFILE *inf;
static int channels;
static float *read_buffer;
static int wait_mode = 1;

int mpx_input_open(char *filename, int srate, int nochan) {
    SF_INFO sfinfo;

    memset(&sfinfo, 0, sizeof(sfinfo));

    if (filename[0] == '-') {
        if (srate <= 0 || nochan <= 0) {
            fprintf(stderr, "Error: --srate and --nochan are required for MPX input from stdin.\n");
            return -1;
        }
        sfinfo.samplerate = srate;
        sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
        sfinfo.channels = nochan;
        sfinfo.frames = 0;

        if (!(inf = sf_open_fd(fileno(stdin), SFM_READ, &sfinfo, 0))) {
            fprintf(stderr, "Error: could not open stdin for MPX input.\n");
            return -1;
        }
        printf("Using stdin for MPX input at %d Hz.\n", srate);
        wait_mode = 1;
    } else {
        if (!(inf = sf_open(filename, SFM_READ, &sfinfo))) {
            fprintf(stderr, "Error: could not open MPX input file %s.\n", filename);
            return -1;
        }
        printf("Using MPX input file: %s\n", filename);
        wait_mode = 0;
    }

    mpx_input_samplerate = sfinfo.samplerate;
    channels = sfinfo.channels;

    printf("MPX input: %d Hz, %d channel(s).\n", mpx_input_samplerate, channels);

    read_buffer = malloc(MPX_INPUT_BUF_LEN * channels * sizeof(float));
    if (read_buffer == NULL) {
        fprintf(stderr, "Error: could not allocate MPX input buffer.\n");
        return -1;
    }

    return 0;
}

int mpx_input_get_samples(double *buffer, int len, float gain) {
    if (inf == NULL) return -1;

    int request = len < MPX_INPUT_BUF_LEN ? len : MPX_INPUT_BUF_LEN;
    sf_count_t read_count = sf_readf_float(inf, read_buffer, request);

    if (read_count < 0) {
        fprintf(stderr, "Error reading MPX input.\n");
        return -1;
    }

    if (read_count == 0) {
        if (sf_seek(inf, 0, SEEK_SET) < 0) {
            if (wait_mode) {
                return 0;
            } else {
                fprintf(stderr, "Could not rewind MPX input file, terminating.\n");
                return -1;
            }
        }
        read_count = sf_readf_float(inf, read_buffer, request);
        if (read_count <= 0) return -1;
    }

    for (int i = 0; i < read_count; i++) {
        buffer[i] = (double)read_buffer[i * channels] * gain;
    }

    return (int)read_count;
}

int mpx_input_close() {
    if (read_buffer != NULL) {
        free(read_buffer);
        read_buffer = NULL;
    }
    if (inf != NULL) {
        sf_close(inf);
        inf = NULL;
    }
    return 0;
}
