/**************************************************
    DansTuner source, copyright (C) 2002 Dan Frankowski

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**************************************************/

/*************************************************
    This is code modified from PortAudio's patest_record.c.
**************************************************/


#ifndef __AUDIOSTREAMS_H__
#define __AUDIOSTREAMS_H__

/* Select sample format. */
#if 1
/* Audacity's Autocorrelation wants a float */
#define PA_SAMPLE_TYPE  paFloat32
#define SAMPLE_FORMAT "%f"
typedef float SAMPLE;
#elif 0
/* Audacity's ComputeSpectrum wants a signed short-- 16 bit integer */
#define PA_SAMPLE_TYPE  paInt16
#define SAMPLE_FORMAT "%d"
typedef short SAMPLE;
#else
#define PA_SAMPLE_TYPE  paUInt8
#define SAMPLE_FORMAT "%d"
typedef unsigned char SAMPLE;
#endif

#include "portaudio.h"   // For PortAudioStream

// This class encapsulates the init/terminate pair needed for Portaudio
// I also hung off a little info about the default devices
class PortAudioInit {
public:
  PortAudioInit() {
    m_err = Pa_Initialize();
  }
  ~PortAudioInit() {
    Pa_Terminate();
  }

  // Returns any error initializing
  const PaError& error() const { return m_err; }

  double deviceSuggestedSampleRate() const;
  const char * deviceName() const;
private:
  PaError m_err;
};

typedef struct
{
  int          frameIndex;  /* Index into sample array. */
  int          maxFrameIndex;
  int          samplesPerFrame;
  SAMPLE      *recordedSamples;
} paRecData;

class recordStream {
public:
  recordStream();
  ~recordStream();
  bool open(const PortAudioInit &i);
  bool close();

  const char * getErrStr() const {
    return m_err_str;
  }

  bool audioSoFar(float& window_ms,
                  SAMPLE &max, SAMPLE &avg_abs,
                  float& bestpeak_freq, float& perfect_freq, float& centsOff,
                  char *& pitchname);

  double m_sample_rate;
private:
  paRecData data;
  PortAudioStream *record_stream;
  const char *m_err_str;
};

typedef struct
{
  int          samplesPerFrame;
  float        currentFreq;
  int          currentPhase;
  float        maxAmplitude;
  double       sample_rate;
} paPlayData;

class playStream {
public:
  playStream();
  ~playStream();
  bool open(const PortAudioInit &i);
  bool close();

  const char * getErrStr() const {
    return m_err_str;
  }

  void SetFrequency(const float& frequency);
  void SetVolume(const float& amplitude);
private:
  paPlayData data;
  PortAudioStream *play_stream;
  double m_sample_rate;
  const char * m_err_str;
};

#endif
