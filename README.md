PiFmMpx
=========


## FM transmitter for pre-built composite MPX (MicroMPX / Stereo Tool) on the Raspberry Pi

PiFmMpx is primarily designed to **transmit a pre-built composite MPX signal** coming from an external encoder such as [MicroMPX](https://www.thimeo.com/micrompx/) or [Stereo Tool](https://www.stereotool.com/). The encoder does all the heavy lifting (stereo encoding, RDS injection, audio processing, FM pre-emphasis, peak limiting), and PiFmMpx simply pumps the resulting composite into the FM carrier - yielding a signal as clean as the encoder is capable of producing, with very low CPU usage on the Pi.

For convenience, PiFmMpx also retains the original PiFmAdv "internal modulation" mode (`--audio`) that generates its own mono / stereo + RDS multiplex from a plain audio file or PCM stream - useful for quick tests without an external encoder, but lower quality and much more CPU-intensive than the MPX path.

PiFmMpx modulates the PLLC instead of the clock divider for better signal purity, which means that the signal is also less noisy. This has a great impact on stereo as it's reception is way better.

For the PLLC modulation to be stable there is one additional step to do. Due to the low voltage detection the PLLC frequency can be reduced to safe value in an attempt to prevent crashes. When this happens, the carrier freqency changes based on the original GPU frequency.
To prevent this, we can easily change the GPU freqency to match the safe frequency. Now when due to the low voltage detection the PLLC frequency changes to safe value, nothing happens as the normal value and safe value are the same.

Simply add `gpu_freq=250` to `/boot/config.txt` (or `/boot/firmware/config.txt` on recent Raspberry Pi OS). If you plan to use the MPX input mode (see below), also set `core_freq=250` and `core_freq_min=250` so PLLD stays stable - otherwise the DMA rate will drop and the transmitted audio will sound slow.

PiFmMpx is a fork of [Miegl/PiFmAdv](https://github.com/Miegl/PiFmAdv), itself based on the FM transmitter created by [Oliver Mattos and Oskar Weigl](http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter), and later adapted to using DMA by [Richard Hirst](https://github.com/richardghirst). Christophe Jacquet adapted it and added the RDS data generator and modulator. Miegl improved the signal quality by modulating the PLLC instead of the clock divider. The transmitter uses the Raspberry Pi's PWM generator to produce VHF signals.

It is compatible with the Raspberry Pi 1, 2, 3 and 4 (including 64-bit / aarch64 builds).

![](doc/vfd_display.jpg)

PiFmMpx has been developed for experimentation only. It is not a media center, it is not intended to broadcast music to your stereo system. See the [legal warning](#warning-and-disclaimer).

## How to use it?

PiFmMpx depends on the `sndfile` library. To install this library on Debian-like distributions, for instance Raspbian, run `sudo apt-get install libsndfile1-dev`.

PiFmMpx uses the SoX Resampler for high-quality resampling. On Debian-based distributions, run `sudo apt install libsoxr-dev` to install the dependency.

PiFmMpx also depends on the Linux `rpi-mailbox` driver, so you need a recent Linux kernel. The Raspbian releases from August 2015 have this.

**Important.** Binaries are model-specific (the Makefile auto-detects Pi 1 / Pi 2-3 / Pi 4 and aarch64 from `uname -m`/`lscpu`). Always re-compile when switching models, so do not skip the `make clean` step in the instructions below!

Clone the source repository and run `make` in the `src` directory:

```bash
git clone https://github.com/Tratosca/PiFmMpx.git
cd PiFmMpx/src
make clean
make
```

### Quick start: transmit a pre-built MPX composite (primary use case)

If you already have a composite MPX signal (from MicroMPX, Stereo Tool, an SDR capture, a `.wav` recording, ...), use `--mpxin`. The radiofrequency signal is emitted on GPIO 4 (pin 7 on header P1).

**From a pre-recorded MPX `.wav` file**, sample rate auto-detected from the header:

```bash
sudo ./pi_fm_mpx --mpxin composite.wav --freq 95.0 --dev 75000
```

**From standard input (raw PCM)** - required when piping live from another process. You must specify `--srate` and `--nochan` because raw streams have no header:

```bash
some_decoder | sudo ./pi_fm_mpx --mpxin - --srate 192000 --nochan 1 --freq 95.0 --dev 75000
```

**Live from MicroMPX through ALSA loopback** (full setup in the [MPX composite input](#mpx-composite-input-micrompx--stereo-tool) section):

```bash
arecord -D hw:0,1,0 -f FLOAT_LE -r 192000 -c 1 -q | \
  sox -t raw -r 192000 -c 1 -b 32 -e float - \
      -t raw -r 192000 -c 1 -b 16 -e signed-integer - | \
  sudo ./pi_fm_mpx --mpxin - --srate 192000 --nochan 1 --freq 95.0 --dev 75000
```

`--dev 75000` matches the standard 75 kHz peak deviation used in most countries (the legacy default of 50 kHz is kept for backwards compatibility but is wrong for FM broadcast).

In `--mpxin` mode the DMA rate follows the input sample rate, so a 192 kHz composite is played back at 192 kHz (instead of the 228 kHz used for internal MPX generation). The internal stereo encoder, audio filter and RDS generator are completely bypassed - RDS, if present, must already be in the composite.

### Internal modulation mode (no external encoder)

If you don't have an encoder and just want to broadcast a plain audio file with on-the-fly stereo encoding and RDS, use `--audio`. To test stereo, the file `stereo_44100.wav` is provided in the repo.

```bash
sudo ./pi_fm_mpx --audio stereo_44100.wav
```

Carrier-only (no audio, default frequency 87.6 MHz, default PS / RT / PI):

```bash
sudo ./pi_fm_mpx
```

### All options

All arguments are optional:

* `--freq` specifies the carrier frequency (in MHz). Default 87.6. Example: `--freq 87.6`.
* `--audio` specifies an audio file to play as audio. The sample rate does not matter: PiFmMpx will resample and filter it. If a stereo file is provided, PiFmMpx will produce an FM-Stereo signal. Example: `--audio sound.wav`. The supported formats depend on `libsndfile`. This includes WAV and Ogg/Vorbis (among others) but not MP3. Specify `-` as the file name to read audio data on standard input (useful for piping audio into PiFmMpx, see below).
* `--mpxin` specifies a pre-built composite MPX signal to transmit as-is. Bypasses all internal audio processing, stereo encoding and RDS generation - useful when fed by an external encoder like MicroMPX or Stereo Tool. The input must be mono PCM (typically 192 kHz). Specify `-` to read from stdin, together with `--srate` and `--nochan`. Mutually exclusive with `--audio`. Example: `--mpxin composite.wav`. See the [MPX composite input](#mpx-composite-input-micrompx--stereo-tool) section below.
* `--srate` sample rate (in Hz) of the raw stdin stream when using `--mpxin -`. Required for headerless input. Example: `--srate 192000`.
* `--nochan` number of channels of the raw stdin stream when using `--mpxin -`. Composite MPX is mono. Example: `--nochan 1`.
* `--pi` specifies the PI-code of the RDS broadcast. 4 hexadecimal digits. Default `1234`. Example: `--pi FFFF`.
* `--ps` specifies the station name (Program Service name, PS) of the RDS broadcast. Limit: 8 characters. Default `PiFmMpx`. Example: `--ps RASP-PI`.
* `--rt` specifies the radiotext (RT) to be transmitted. Limit: 64 characters. Example:  `--rt 'Hello, world!'`.
* `--af` specifies alternative frequencies (AF). Example:  `--af 107.9 --af 99.2`.
* `--pty` specifies the program type. 0 - 31. Default 15. Example: `--pty 10` (EU: Pop music). See https://en.wikipedia.org/wiki/Radio_Data_System for more program types.
* `--tp` specifies if the program carries traffic information. Default 1. Example `--tp 0`.
* `--dev` specifies the frequency deviation (in Hz). Default 50000. Example `--dev 75000`.
* `--mpx` specifies the input/output MPX gain. Default 20. In `--mpxin` mode, a value of `20` maps full-scale ±1.0 input to full deviation. Example `--mpx 30`.
* `--power` specifies the drive strenght of gpio pads. 0 = 2mA ... 7 = 16mA. Default 7. Example `--power 5`.
* `--gpclk` specifies the General Purpose Clock (GPCLK) to use. Available options are 0, 1 and 2. Default 0.
* `--gpio` specifies the GPIO pin used for transmitting. Can be specified multiple times to transmit on multiple pins at once. Default 4. Can be used in combination with `--gpclk`. Example `--gpio 20`.
* `--pll` specifies the PLL to modulate. PLLC is used by default, PLLA doesn't seem to work on Raspberry Pi 4. Available options: `a` and `c`.
* `--div` overrides the automatic VCO/output divider selection. Use only if the auto-tuning algorithm fails. Example `--div 14`.
* `--prediv` PLL pre-divider. Default `1` (Pi 1/2/3) and `2` (Pi 4). Adjust only if you know what you're doing.
* `--cutoff` specifies the cutoff frequency (in Hz) used by PiFmMpx's internal lowpass filter (in `--audio` mode only). Default 15000. Values greater than 15000 are not compliant. Use carefully.
* `--preemph` specifies which preemph should be used, since it differs from location. For Europe choose 'eu' (3185), for the US choose 'us' (2120). A custom cutoff (in Hz) is also accepted.
* `--ctl` specifies a named pipe (FIFO) to use as a control channel to change PS, RT, TA and PTY at run-time (see below).
* `--ppm` specifies your Raspberry Pi's oscillator error in parts per million (ppm), see below.
* `--rds` enables (`1`) or disables (`0`) the RDS broadcast. Default 1. Ignored in `--mpxin` mode (the RDS subcarrier must already be in the composite signal).
* `--wait` specifies whether PiFmMpx should wait for the the audio pipe or terminate as soon as there is no audio. It's set to 1 by default. 

By default the PS is set to `PiFmMpx`. Use `--ps` to set another 8-character station name, or `--ctl` to change it at run-time.


### Clock calibration (only if experiencing difficulties)

The RDS standards states that the error for the 57 kHz subcarrier must be less than ± 6 Hz, i.e. less than 105 ppm (parts per million). The Raspberry Pi's oscillator error may be above this figure. That is where the `--ppm` parameter comes into play: you specify your Pi's error and PiFmMpx adjusts the clock dividers accordingly.

In practice, PiFmMpx works okay even without using the `--ppm` parameter. Receivers are usually more tolerant than stated in the RDS spec.

One way to measure the ppm error is to play the `pulses.wav` file: it will play a pulse for precisely 1 second, then play a 1-second silence, and so on. Record the audio output from a radio with a good audio card. Say you sample at 44.1 kHz. Measure 10 intervals. Using [Audacity](http://audacity.sourceforge.net/) for example determine the number of samples of these 10 intervals: in the absence of clock error, it should be 441,000 samples. With my Pi, I found 441,132 samples. Therefore, my ppm error is (441132-441000)/441000 * 1e6 = 299 ppm, **assuming that my sampling device (audio card) has no clock error...**


### Piping audio into PiFmMpx

If you use the argument `--audio -`, PiFmMpx reads audio data on standard input. This allows you to pipe the output of a program into PiFmMpx. For instance, this can be used to read MP3 files using Sox:

```
sox -t mp3 http://www.linuxvoice.com/episodes/lv_s02e01.mp3 -t wav -  | sudo ./pi_fm_mpx --audio -
```

Or to pipe the AUX input of a sound card into PiFmMpx:

```
sudo arecord -fS16_LE -r 44100 -Dplughw:1,0 -c 2 -  | sudo ./pi_fm_mpx --audio -
```


### MPX composite input (MicroMPX / Stereo Tool)

A composite MPX signal is the full FM multiplex: 50 Hz – 15 kHz mono sum, 19 kHz pilot tone, suppressed-carrier 38 kHz stereo difference, RDS at 57 kHz, plus optional RDS2 / SCA subcarriers. External encoders (MicroMPX, Stereo Tool) produce this stream and apply broadcast-grade processing, peak limiting and pre-emphasis. PiFmMpx transmits the composite as-is - see the [Quick start](#quick-start-transmit-a-pre-built-mpx-composite-primary-use-case) above for the basic command syntax.

The `--mpx` parameter controls the input gain (default `20` → full-scale ±1.0 input maps to full deviation). Adjust `--dev` to match the standard FM deviation (75 kHz in most countries).

**Important:** MPX mode requires a stable PLLD, so make sure you have pinned the GPU / core frequency in `/boot/firmware/config.txt` as explained at the top of this README; otherwise the DMA rate will drop under load and the transmitted audio will sound slow and lose stereo lock.

#### Live from MicroMPX through ALSA loopback

The recommended setup runs the [MicroMPX decoder](https://www.thimeo.com/micrompx/) on the same Pi and pipes its output through the kernel ALSA loopback (`snd-aloop`) into PiFmMpx.

```bash
# 1. Enable the ALSA loopback module (once)
sudo modprobe snd-aloop
echo snd-aloop | sudo tee -a /etc/modules-load.d/aloop.conf

# 2. Find the loopback card index
aplay -l | grep -i loopback
#   → e.g. "card 2: Loopback [Loopback], device 0: ..."   < the card index is 2 here

# 3. In the MicroMPX web UI (http://<pi>:8080), set
#    "FM Output device" = "Loopback: PCM (hw:<card>,0)"
#    (use the card index you found above)

# 4. Start the decoder (in another shell or as a service)
./MicroMPX_Decoder &

# 5. Capture the loopback's other side and feed PiFmMpx.
#    arecord reads device hw:<card>,1,0 (subdevice 1 is the read side).
#    sox converts MicroMPX's FLOAT_LE samples to S16_LE, which is what
#    --mpxin's raw stdin mode expects.
arecord -D hw:<card>,1,0 -f FLOAT_LE -r 192000 -c 1 -q | \
  sox -t raw -r 192000 -c 1 -b 32 -e float - \
      -t raw -r 192000 -c 1 -b 16 -e signed-integer - | \
  sudo ./pi_fm_mpx --mpxin - --srate 192000 --nochan 1 --freq 95.0 --dev 75000
```

If the Pi has no other sound cards, the loopback usually lands at card 0; with onboard HDMI / 3.5 mm audio enabled it lands at card 2 (depending on `dtparam=audio=on`). Run `aplay -l` to be sure.

#### Live from Stereo Tool

Stereo Tool can output composite over the same ALSA loopback. The pipeline is identical to the MicroMPX one - point Stereo Tool's FM output to the loopback playback side and `arecord` from the capture side into PiFmMpx.

#### From a pre-recorded composite

If you have a `.wav` of an MPX composite (e.g. captured via the same ALSA loopback to file with `arecord -D hw:<card>,1,0 -f S16_LE -r 192000 -c 1 capture.wav`), play it back with:

```bash
sudo ./pi_fm_mpx --mpxin capture.wav --freq 95.0 --dev 75000
```

Sample rate is read from the WAV header - `--srate` / `--nochan` are only needed for raw stdin.


### Changing PS, RT, TA and PTY at run-time (`--audio` mode only)

> **Not applicable in `--mpxin` mode.** When transmitting a pre-built composite, the RDS subcarrier is already inside the multiplex produced by your external encoder: PiFmMpx's internal RDS generator is bypassed and the `--ctl` commands have no effect on the air. Use your encoder's own RDS control (Stereo Tool's HTTP API, MicroMPX's RDS settings, ...) instead.

You can control PS, RT, TA (Traffic Announcement flag) and PTY (Program Type) at run-time using a named pipe (FIFO). For this run PiFmMpx with the `--ctl` argument.

Example:

```
mkfifo rds_ctl
sudo ./pi_fm_mpx --ctl rds_ctl
```

Then you can send “commands” to change PS, RT, TA and PTY:

```
cat >rds_ctl
PS MyText
RT A text to be sent as radiotext
PTY 10
TA ON
PS OtherTxt
TA OFF
...
```

Every line must start with either `PS`, `RT`, `TA` or `PTY`, followed by one space character, and the desired value. Any other line format is silently ignored. `TA ON` switches the Traffic Announcement flag to *on*, any other value switches it to *off*.


## Warning and Disclaimer

PiFmMpx is an **experimental** program, designed **only for experimentation**. It is in no way intended to become a personal *media center* or a tool to operate a *radio station*, or even broadcast sound to one's own stereo system.

In most countries, transmitting radio waves without a state-issued licence specific to the transmission modalities (frequency, power, bandwidth, etc.) is **illegal**.

Therefore, always connect a shielded transmission line from the Raspberry Pi directly
to a radio receiver, so as **not** to emit radio waves. Never use an antenna.

Even if you are a licensed amateur radio operator, using PiFmMpx to transmit radio waves on ham frequencies without any filtering between the RaspberryPi and an antenna is most probably illegal because the square-wave carrier is very rich in harmonics, so the bandwidth requirements are likely not met.

I could not be held liable for any misuse of your own Raspberry Pi. Any experiment is made under your own responsibility.


## Tests

PiFmAdv (the upstream project) was successfully tested with these RDS-able devices:

* a Sony ICF-C20RDS alarm clock from 1995,
* a Sangean PR-D1 portable receiver from 1998, and an ATS-305 from 1999,
* a Samsung Galaxy S2 mobile phone from 2011,
* a Philips MBD7020 hifi system from 2012,
* a Silicon Labs [USBFMRADIO-RD](http://www.silabs.com/products/mcu/Pages/USBFMRadioRD.aspx) USB stick, employing an Si4701 chip, and using my [RDS Surveyor](http://rds-surveyor.sourceforge.net/) program,
* a “PCear Fm Radio”, a Chinese clone of the above, again using RDS Surveyor.

Reception works perfectly with all the devices above. RDS Surveyor reports no group errors.

![](doc/galaxy_s2.jpg)


### CPU Usage

Indicative single-core CPU usage on a Raspberry Pi 3B+ (one core = 25% of the four-core total reported by `top`):

| Mode | `pi_fm_mpx` | Total pipeline |
| --- | --- | --- |
| `--mpxin` from MicroMPX (192 kHz, sox + arecord) | ~10% | ~15% (pi_fm_mpx + sox; MicroMPX_Decoder runs separately) |
| `--mpxin` from a pre-recorded WAV | ~10% | ~10% |
| `--audio` mono | ~33% | ~33% |
| `--audio` stereo | ~40% | ~40% |
| no audio (carrier only) | ~9% | ~9% |

In `--audio` mode, CPU usage is high because the program upsamples the input (whatever the file's sample rate) to its internal 228 kHz rate and applies a 60-tap FIR filter, then synthesises the stereo MPX and RDS subcarriers in real time. In `--mpxin` mode the DMA rate follows the input sample rate (e.g. 192 kHz) and no upsampling, filtering, stereo encoding or RDS generation is performed - the program just reshuffles input samples into the DMA buffer.

The MicroMPX decoder itself is the heavy process in a typical live setup (~75% on a Pi 3B+ at 192 kHz), but it's independent of PiFmMpx - you pay this cost regardless of how you transmit the resulting composite.

## Design

The RDS data generator lies in the `rds.c` file.

The RDS data generator generates cyclically four 0A groups (for transmitting PS), and one 2A group (for transmitting RT). In addition, every minute, it inserts a 4A group (for transmitting CT, clock time). `get_rds_group` generates one group, and uses `crc` for computing the CRC.

To get samples of RDS data, call `get_rds_samples`. It calls `get_rds_group`, differentially encodes the signal and generates a shaped biphase symbol. Successive biphase symbols overlap: the samples are added so that the result is equivalent to applying the shaping filter (a [root-raised-cosine (RRC) filter ](http://en.wikipedia.org/wiki/Root-raised-cosine_filter) specified in the RDS standard) to a sequence of Manchester-encoded pulses.

The shaped biphase symbol is generated once and for all by a Python program called `generate_waveforms.py` that uses [Pydemod](https://github.com/ChristopheJacquet/Pydemod), one of Christophe Jacquet's other software radio projects. This Python program generates an array called `waveform_biphase` that results from the application of the RRC filter to a positive-negative impulse pair. *Note that the output of `generate_waveforms.py`, two files named `waveforms.c` and `waveforms.h`, are included in the Git repository, so you don't need to run the Python script yourself to compile PiFmMpx.*

In `--audio` mode, the program samples all signals at 228 kHz, four times the RDS subcarrier's 57 kHz. In `--mpxin` mode, the DMA rate follows the input sample rate of the composite signal.

The FM multiplex signal (baseband signal) is generated by `fm_mpx.c`. This file handles the upsampling of the input audio file to 228 kHz, and the generation of the multiplex: unmodulated left+right signal (limited to 15 kHz), possibly the stereo pilot at 19 kHz, possibly the left-right signal, amplitude-modulated on 38 kHz (suppressed carrier) and RDS signal from `rds.c`. Upsampling is performed using a zero-order hold followed by an FIR low-pass filter of order 60. The filter is a sampled sinc windowed by a Hamming window. The filter coefficients are generated at startup so that the filter cuts frequencies above the minimum of:
* the Nyquist frequency of the input audio file (half the sample rate) to avoid aliasing,
* 15 kHz, the bandpass of the left+right and left-right channels, as per the FM broadcasting standards.

The pre-built composite MPX path lives in `mpx_input.c` / `mpx_input.h`. It bypasses `fm_mpx.c` and `rds.c` entirely and feeds raw samples to the DMA buffer at the rate of the input file (or `--srate` for raw stdin).

The samples are played by `pi_fm_adv.c` that is adapted from Richard Hirst's [PiFmDma](https://github.com/richardghirst/PiBits/tree/master/PiFmDma). It supports the 228 kHz internal rate as well as arbitrary rates for the MPX composite input.


### References

* [EN 50067, Specification of the radio data system (RDS) for VHF/FM sound broadcasting in the frequency range 87.5 to 108.0 MHz](http://www.interactive-radio-system.com/docs/EN50067_RDS_Standard.pdf)


## History

* 2026-04-18: PiFmMpx fork - add `--mpxin` MPX composite input mode (MicroMPX / Stereo Tool), with optional raw stdin (`--srate`, `--nochan`)
* 2023-09-26: support multiple board revisions
* 2023-09-24: aarch64 (64-bit) build support
* 2022-04-16: `--prediv` option, tuning fix on Raspberry Pi 4
* 2022-04-15: `--gpclk` and `--pll` options, Raspberry Pi 4 support fixes
* 2022-02-04: `--gpio` accepts multiple values to transmit on several pins at once; SoX resampler for high-quality resampling; control pipe improvements
* 2015-09-05: support for the Raspberry Pi 2
* 2014-11-01: support for toggling the Traffic Announcement (TA) flag at run-time
* 2014-10-19: bugfix (cleanly stop the DMA engine when the specified file does not exist, or it's not possible to read from stdin)
* 2014-08-04: bugfix (ppm now uses floats)
* 2014-06-22: generate CT (clock time) signals, bugfixes
* 2014-05-04: possibility to change PS and RT at run-time
* 2014-04-28: support piping audio file data to standard input
* 2014-04-14: new release that supports any sample rate for the audio input, and that can generate a proper FM-Stereo signal if a stereophonic input file is provided
* 2014-04-06: initial release, which only supported 228 kHz monophonic audio input files

--------

PiFmMpx © 2026, based on PiFmAdv © [Miegl](https://miegl.cz) & [Christophe Jacquet](http://www.jacquet80.eu/) (F8FTK), 2014-2017. Released under the GNU GPL v3.
