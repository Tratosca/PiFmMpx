/*
    PiFmAdv - Advanced FM transmitter for the Raspberry Pi
    Copyright (C) 2017 Miegl

    See https://github.com/Miegl/PiFmAdv
*/

#ifndef MPX_INPUT_H
#define MPX_INPUT_H

extern int mpx_input_open(char *filename, int srate, int nochan);
extern int mpx_input_get_samples(double *buffer, int len, float gain);
extern int mpx_input_close();
extern int mpx_input_samplerate;

#endif /* MPX_INPUT_H */
