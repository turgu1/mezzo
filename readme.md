# Mezzo: Sound Font V2 Sampling Based Synthesizer

Important: This software can now (as of July 23rd, 2018) be used to play music. There is still some debugging being done and some features to be added, so stay tuned.

Note: The documentation is still work in progress...

Mezzo is a C++ application that runs on a Raspberry PI 2 or 3. It is a kind of a "fire and forget" application that would permit a Raspberry PI 2/3 device to be permanently hooked to a midi controller and have Mezzo started automatically at bootstrap, without requiring any screen, mouse or typewriter keyboard. Very few changes, if any, would be required to port it to other platforms. It is a console based application that keep as much resources as possible available for playing sounds. So far, no trial have been made to use it at the same time of a graphical interface with the user.

Mezzo has the following characteristics:

* Up to 620 voices polyphony (from experiment with a Yamaha Grand C7 preset). Mileage may vary depending on the resources available and features selected in the SoundFont presets.
* Very low latency (lower than 6ms)
* Algebraic Reverb filter, based on the FreeVerb algorithm
* 7 band digital output equalizer (work in progress)
* High optimization for the Raspberry Pi using ARM NEON intrinsic DSP instructions and for Intel processors using SSE DSP intrinsics (V3.x).
* Multithreaded application, to optimize the use of available hardware thread available.
* Console based, no graphics, fire and forget application. Control is done through a simple interactive text-based menu or a Midi Keyboard Controller.
* Minimal interactive mode for initial setup and debugging purposes
* MIDI channel listening control
* Free and Open source. You can do what you want with it. See the licensing section for details
* Well documented application (in progress). Looking at the code, you can learn a bit on how such a C++ application can be designed

Mezzo 
## Some background

I was used to play piano (for myself as an amateur) on a Roland A-88 MIDI controller, with a MacBook running Kontakt with some piano sample libraries in the back-end. The sound quality obtained with this combination was excellent, but for a computer science addict like me, it lacked my own touch of design and satisfaction in making my own instrument (It was also cumbersome of installing the laptop and the cables every time I wanted to play...).

I was ready to buy a dedicated sampling computer or better, build my own. When the Raspberry Pi was first introduced, I thought it would be a good start at building my own music computer, but it was lacking some important features: enough USB ports (at least 3: MIDI controller access, external DAC, WIFI dongle), memory and processing power to permit a reasonable amount of good quality samples and polyphonic voices to be played. The first music related projects that started to emerge shown the device limitation and simply pushed to the right my intent.

With the arrival of the Raspberry Pi 2, the new capability of the device (faster multi-core processor, more memory) triggered again my interest in this project and I started to build something useful, it was named PIano (On Github: [https://github.com/turgu1/Piano]). It was fun to built and it is fun to play piano with my own stuff. But it was lacking of better support of standardized open audio samples formatting (namely SoundFont) and many features that you would find in a modern synth (namely envelope, filtering, modulation controls).

With the arrival of the Raspberry Pi 3B+, I expected to get enough processing power to build a new version of PIano. This one is named Mezzo.

This is certainly not the best sampling application available. My intent is to bridge the gap with other more sophisticated alternatives. Some decisions taken also reflect the "good enough selection" I made, considering some arbitrary level of satisfaction of my own or unavailability of time for me to address some of the shortcomings encountered more closely.

All along the development of Mezzo, I've been using it extensively to ensure it won't show misbehaving states on a dedicated Raspberry PI that is run in console mode. As it's still work in progress, I expect some issues yet to be find and will fix them as soon as possible once discovered.

The Raspberry Pi 3B+ is the target. But it can be compiled on any other Linuxish platform (even on MaxOS). Mileage may vary depending on the platform performances (CPU, memory, I/O bandwidth).

## External Devices

As stated, I'm using a Roland A-88 to control Mezzo. This MIDI controller is beneficial as it is equipped with a USB based serial interface. The usual MIDI connectors are also available but unused in this application for simplicity (no need for specific electronic changes to the PI). That controller supplies many knobs and buttons that allow controlling volume, reverb, etc. This is a rather expansive MIDI controller. Other options are available on the market. If you get access to a controller that only use MIDI standard connectors, you may have to built your own hardware interface or acquire a MIDI to USB adapter (search for "midi USB interface" on Amazon). Mezzo also support the used of a sustain pedal hooked to the MIDI controller

For audio output, I've tried at first to use the PI on-board audio device but the sound quality was not adequate. I acquired a MUSE DAC (Digital to Analog Converter based on the famous PCM2704 chip available for 25$ on Amazon) that supply output for headphones, S/PDIF and optical connections that would allow for external audio amplification.

## Installation

Two methods are available to install Mezzo:

* Installation from source code. This method will built the application from the source distribution available on the Internet.
* Installation from a pre-compile version of Mezzo. This method allow for a quicker installation, as no application buildup is required (this remains to be added to the repository).

### Installation from source code

The suite of GNU C++ compiler and libraries is required to build the application. Also, the following libraries have been used to supply interfaces to external resources:

* RtAudio - For ALSA based PCM audio device access
* RtMidi  - Midi ALSA input device interaction (already part of Mezzo source code)
* Boost   - C++ libraries to support some algorithms design

Raspbian is already containing the GNU C++ suite of tools. What remains to be install are some external libraries. The following subsections give commands to be executed in a teminal shell.

### Install Raspbian packages

Starting from a plain vanilla Raspbian distribution, you will need to install the required packages using the following commands, once logged-in with the usual pi username:

```bash
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git librtmidi-dev librtaudio-dev libboost1.62-all-dev libasound2-dev
```

## Retrieving and compiling Mezzo

On the Raspberry Pi, you can then retrieve and compile Mezzo with the following commands:

```bash
git clone https://github.com/turgu1/mezzo.git
cd mezzo
make
```

You then get a binary file in bin/mezzo

## Configuration

This section of the documentation gives the procedure to install Mezzo to make it run on a Raspberry PI. The application is a single binary file named "Mezzo" that is made available in the GitHub directory tree. If you prefer to rebuild the application, the section Compiling will direct you on how to recreate the binary code.

Beyond the application, it will be required to have a sampling library. Many of these libraries are available for free on the internet. A good source of information is located at the following site:

  [http://www.synthfont.com/links_to_soundfonts.html]

Another good sample library site is the following:

  [https://sites.google.com/site/soundfonts4u/]

I'm use to play with the following sample library: [Sal-Stein-Uprights-Detailed-V3.0](https://drive.google.com/file/d/0B3zFERJ2rMQpVUJfMWRqUGRtc3M/view?usp=sharing).

SF2 is a sound font library file format that is easy to read and doesn't require too much resources to extract PCM data. Sampling libraries are usually huge (from hundreds of megabytes to several gigabytes) and would need enough disk space to be kept on the device. You may choose to put your SF2 library on a USB dongle, external disk drive or on the Micro-SD card on which the Raspbian operating system has been installed. My own preference was to use the Micro-SD card as it will not require any external equipment and would allow for raisonable performance in terms of I/O. Using a USB dongle would be more practical only if you intend to use small sampling libraries (say, smaller than around 1GB). It could become impractical performance-wise if disk caching is not sufficient to allow for real-time sample retrieval from the file. Only SF2 files are supported and no provision is made to support other formats.

Mezzo requires a minimum of information to make it happy to start listening to the MIDI controller and prepare the sounds to be sent to the audio device. This information is gathered in a configuration file names .mezzo.conf that must be on the users's main directory. You can copy the supplied one to the directory:

```bash
cp ~/mezzo/mezzo.conf ~/.mezzo.conf
```

You then need to adjust the parameters located in the .mezzo.conf file to your liking. The content is self-explanatory. In particular, you have to identify the soundFont library file (usually with a filename ending with '.sf2') to be loaded at launch time. 

## Licensing

The following applies to all source files, except for [neon_2_sse.h](https://github.com/turgu1/mezzo/src/include/neon_2_sse.h) that is copyright 2012-2018 from Intel Corporation. See that file for the complete copyright notice. It was downloaded from [Intel](https://github.com/intel/ARM_NEON_2_x86_SSE) on GitHub.

[Simplified BSD License]

Copyright (c) 2018, Guy Turcotte
All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

