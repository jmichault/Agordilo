/**************************************************
    copyright (C) 2002 Dan Frankowski
    copyright (C) 2004 Jean Michault

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

#include "audiostreams.h"
#include <math.h>      // For asin(), sin()
#include <string.h>      // For bzero
#include "portaudio.h"
#include <stdlib.h>    // For free()
#include <stdio.h>     // For FILE, etc.
#include "Spectrum.h"  // For Autocorrelation()
#include "FFT.h"       // For M_PI

#define OCTAVE 4
unsigned long WindowSize = 16384 >>  OCTAVE;

//////////////////////////////////////////////////
//
// PortAudioInit
//
//////////////////////////////////////////////////
double PortAudioInit::deviceSuggestedSampleRate() const {
  const PaDeviceInfo *pdi = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice());
  return (pdi->defaultSampleRate>44000.0?44000.0:pdi->defaultSampleRate);

}

const char *
PortAudioInit::deviceName() const {
  const PaDeviceInfo *pdi = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice());
  return pdi->name;
}

//////////////////////////////////////////////////
//
// recordStream
//
//////////////////////////////////////////////////

/* This routine will be called by the PortAudio engine.
 * It may be called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 *
 * This routine takes sound from inputBuffer (the recorded sound)
 * and jams it into a "data" buffer passed in by userData.
 */
static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
			   void *userData )
{
  paRecData *data = (paRecData*)userData;
  SAMPLE *rptr = (SAMPLE*)inputBuffer;
  SAMPLE *wptr = &data->recordedSamples[data->frameIndex * data->samplesPerFrame];
  long framesToCalc;
  long i, j;
  int finished;
  unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;

  (void) outputBuffer; /* Prevent unused variable warnings. */
  (void) timeInfo; /* Prevent unused variable warnings. */
  (void) statusFlags; /* Prevent unused variable warnings. */
	
  if( framesLeft < framesPerBuffer ) {
    framesToCalc = framesLeft;
    finished = 1;
  }
  else {
    framesToCalc = framesPerBuffer;
    finished = 0;
  }

  if( inputBuffer == NULL ) {
    for( i=0; i<framesToCalc; i++ ) {
      for (j=0; j<data->samplesPerFrame; j++) {
        *wptr++ = 0;
      }
    }
  }
  else {
    for( i=0; i<framesToCalc; i++ ) {
      for (j=0; j<data->samplesPerFrame; j++) {
        *wptr++ = *rptr++;
      }
    }
  }
  data->frameIndex += framesToCalc;
  return finished;
}


recordStream::recordStream() {
  data.frameIndex = 0;
  data.samplesPerFrame = 1;    /* 1 for mono input/output, 2 for stereo. */
}

recordStream::~recordStream() {
  if (data.recordedSamples) {
    delete [] data.recordedSamples;
  }
}

bool recordStream::open(const PortAudioInit &init) {
  if (init.error() != paNoError) {
    m_err_str = Pa_GetErrorText(init.error());
    return false;
  }
  m_sample_rate = init.deviceSuggestedSampleRate();

  // Allow buffer space for at least this many seconds
  // If all of this fills up, the record stream will close
  const int NUM_SECONDS = 10;
  data.maxFrameIndex = int(NUM_SECONDS * m_sample_rate);

  int numSamples = data.maxFrameIndex * data.samplesPerFrame;

  data.recordedSamples = new SAMPLE[numSamples];
  if( data.recordedSamples == NULL ) {
    //    printf("Could not allocate record array.\n");
    m_err_str = "Couldn't allocate recording buffer";
    return false;
  }
  for(int i=0; i<numSamples; i++ ) {
    data.recordedSamples[i] = 0;
  }

  /* Open record stream. -------------------------------------------- */
  /*
 PaStreamParameters outputParameters;
 PaStreamParameters inputParameters;
  bzero( &inputParameters, sizeof( inputParameters ) );
  inputParameters.channelCount = 1;
  inputParameters.device = Pa_GetDefaultInputDevice();
  inputParameters.hostApiSpecificStreamInfo = NULL;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice())->defaultLowInputLatency ;
  inputParameters.hostApiSpecificStreamInfo = NULL;
  bzero( &outputParameters, sizeof( outputParameters ) );

 PaError err = 
    Pa_OpenStream(
                  &record_stream,
		  &inputParameters,
                  &outputParameters,
                  m_sample_rate,
                  1024,            // frames per buffer 
                  paClipOff,  // we won't output out of range samples so don't bother clipping them 
                  recordCallback,
                  &data );
  */
  PaError err = Pa_OpenDefaultStream(
                              &record_stream,
			      1,
                              0,
			      paFloat32,
                              m_sample_rate,
                              1024,            /* frames per buffer */
                              recordCallback,
                              &data );
  if( err != paNoError ) goto error;

  err = Pa_StartStream( record_stream );
  if( err != paNoError ) goto error;

  return true;

 error:
  m_err_str = Pa_GetErrorText(err);
  return false;
}

bool recordStream::close() {
  return paNoError == Pa_CloseStream( record_stream );
}

/*
 * This routine analyzes audio data, and displays results.
 */
  float *autocorr = 0;
  int autocorrLen = 0;
bool
analyzeAudio(paRecData& data, int numSamples,
             double sample_rate,
             SAMPLE& max, SAMPLE& average_abs,
             float& bestpeak_freq, float& perfect_freq, float& centsOff,
             char*& pitchname) {
  bool gotSound = false;

  /* Measure peak and average amplitude. */
  {
    SAMPLE val;
    int i;

    max = 0;
    average_abs = 0;
    for( i=0; i<numSamples; i++ ) {
      val = data.recordedSamples[i];
      if( val < 0 ) val = -val; /* ABS */
      if( val > max ) max = val;
      average_abs += val;
    }
    //printf("sample max amplitude = " SAMPLE_FORMAT "\n", max );
    average_abs /= numSamples;
    //printf("sample average_abs = " SAMPLE_FORMAT "\n", average_abs );
    //    fflush(stdout);
  }
//jmt
//printf("analyzeAudio : NS=%d,max=%f,av=%f\n",numSamples,max,average_abs);
  /* Write out the analyzed spectrum of the recorded data */
  autocorrLen = 0;
//  if (autocorr) free(autocorr);

  // Gotta have something to go on
  if ((numSamples > 0) && (average_abs > 0)) {
    /* Get an autocorrelation-- The highest peak is the pitch */
    if (Autocorrelation( data.recordedSamples, numSamples,
                         WindowSize /* window size in samples */,
                         &autocorr, autocorrLen)) {
      /* Find the tallest peak, and print its frequency and pitch */
      float bestpeak_x = bestPeak2(autocorr, autocorrLen, sample_rate);
      if (bestpeak_x > 0) {
        bestpeak_freq = sample_rate / bestpeak_x;

        if ((bestpeak_freq > 1) && (bestpeak_freq < 20000)) {
          float bestpeak_pitch = Freq2Pitch(bestpeak_freq);
          int bestpeak_pitch_int = int(bestpeak_pitch+0.5);
          perfect_freq = Pitch2Freq(int(bestpeak_pitch+0.5));
          pitchname = PitchName(bestpeak_pitch_int, 0 /* No flats */);

          // The pitch might still be stupid because max was really
          // small, hence we're just scraping up noise, but I'll
          // leave that to the client to decide
          gotSound = true;

          centsOff = (bestpeak_pitch - bestpeak_pitch_int) * 100;
        }
      }
    }
  }

  if (!gotSound) {
    // Silence or weirdness, hence forget about pitch
    perfect_freq = 0;
    bestpeak_freq = 0;
    centsOff = 0;
    pitchname = "";
  }

//  free(autocorr);

  return gotSound;
}

bool recordStream::audioSoFar(float& window_ms,
                              SAMPLE &max, SAMPLE &avg_abs,
                              float& bestpeak_freq, float& perfect_freq, float& centsOff,
                              char *& pitchname) {
  int numSamplesSoFar = data.frameIndex * data.samplesPerFrame;

  window_ms = data.frameIndex / float(m_sample_rate) * 1000;

  bool success = analyzeAudio(data, numSamplesSoFar,
                              m_sample_rate,
                              max, avg_abs,
                              bestpeak_freq, perfect_freq, centsOff,
                              pitchname);

  /* Return the frame index to 0, so we keep going with only new sound */
  data.frameIndex = 0;

  return success;
}


//////////////////////////////////////////////////
//
// playStream
//
//////////////////////////////////////////////////
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int playCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
			const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                        void *userData )
{
  paPlayData *data = (paPlayData*)userData;
  float& currentFreq = data->currentFreq;
  int& currentPhase = data->currentPhase;

#if 0
  {
    FILE *fp = fopen("foo.txt", "a");
    fprintf(fp, "callback: currentFreq is %f lastFreq is %f\n",
            currentFreq, lastFreq);
    fclose(fp);
  }
#endif

  static float lastFreq = 0;
  static SAMPLE lastLevel = 0;

  // Should assert here that fadeWindow < "frames per buffer"
  // when the callback is registered
  const unsigned int fadeWindow = 512;
  int fade = 0;

  float newFreq = currentFreq;
  if (currentFreq != lastFreq) {
    // Adjust the phase so that this sine wave continues from the
    // previous sine wave in a way that doesn't produce discontinuities
    // (i.e. crackles)

    // Keep the last frequency in order to fade
    currentFreq = lastFreq;

    fade = 1;
  }

  SAMPLE *wptr = (SAMPLE*)outputBuffer;

  /* Prevent unused variable warnings. */
  (void) inputBuffer;
  (void) timeInfo;
  (void) statusFlags;

  float fadeAdjustment = data->maxAmplitude;
  for(unsigned int i=0; i<framesPerBuffer; i++ ) {
    float floatPhase = currentPhase * 2 * M_PI / data->sample_rate;
    SAMPLE level = 0.5 + sin(currentFreq * floatPhase) / 2;

    if (fade) {

      if (i <= (fadeWindow / 2)) {
        fadeAdjustment = data->maxAmplitude
          * (1 - float(i) / (fadeWindow / 2));
        // printf ("DN:l %.2f ", level * fadeAdjustment);
      }
      else if (i <= fadeWindow) {
        fadeAdjustment = data->maxAmplitude
          * ((i - (fadeWindow / 2)) / float(fadeWindow / 2));
        // printf ("UP:l %.2f ", level * fadeAdjustment);
      }

      // Switch over to new frequency at sample (fadeWindow / 2)
      if (i == (fadeWindow / 2)) {
        float floatPhase = asin(2 * (level-0.5)) / newFreq;

#if 0
        printf("i = %d\n", i);
        printf("  old phase: %d  old level: %f  old freq: %f\n",
               currentPhase, level, currentFreq);
#endif

        currentPhase = int(floatPhase * data->sample_rate / (2 * M_PI));
        currentFreq = newFreq;

#if 0
        printf("  new phase: %d  new level: %f  new freq: %f\n",
               currentPhase, level, currentFreq);
#endif

#if 0
        // This should be an assert(), or wxASSERT() or something
        // Should be the same
        SAMPLE newLevel = 0.5 + sin(currentFreq * floatPhase) / 2;
        if (fabs(newLevel - level) > 0.00001) {
          printf("Bad: level (%f) != newLevel (%f)\n", level, newLevel);
        }
#endif
      }
      if (i == fadeWindow) {
        fade = 0;
      }
    }

    for (int j=0; j<data->samplesPerFrame; j++) {
      *wptr++ = level * fadeAdjustment;
    }

    currentPhase ++;
    // Phase is modulo m_sample_rate
#if 0
    if (0 == (currentPhase % sample_rate)) {
      //      currentPhase = 0;
      //printf("Current phase is 0.\n");
    }
#endif
    //    if (0 == (i % 1000)) printf(" phase = %.2f level = %.2f\n",
    //                                currentPhase, level);
  }

  lastLevel = *(wptr-1);
  lastFreq = currentFreq;
  //  printf("lastL is %.2f  ", lastLevel);

  return 0; // 0 means continue playing
}

playStream::playStream() {
  data.samplesPerFrame = 1;    /* 1 for mono input/output, 2 for stereo. */
  data.currentFreq = 0;
  data.currentPhase = 0;
  data.maxAmplitude = 0.4;
}

playStream::~playStream() {
}

bool playStream::open(const PortAudioInit &init) {
  if (init.error() != paNoError) {
    m_err_str = Pa_GetErrorText(init.error());
    return false;
  }
  data.sample_rate = m_sample_rate = init.deviceSuggestedSampleRate();
 PaStreamParameters outputParameters;
 PaStreamParameters inputParameters;
  bzero( &inputParameters, sizeof( inputParameters ) );
  bzero( &outputParameters, sizeof( outputParameters ) );
  outputParameters.channelCount = 1;
  outputParameters.device = Pa_GetDefaultOutputDevice();
  outputParameters.hostApiSpecificStreamInfo = NULL;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice())->defaultLowOutputLatency ;
  outputParameters.hostApiSpecificStreamInfo = NULL; //See you specific host's API docs for info on using this field


/*
  PaError err = Pa_OpenStream(
                              &play_stream,
			      &inputParameters,
                              &outputParameters,
                              m_sample_rate,
                              1024,            // frames per buffer 
                              paNoFlag,        // let it do clipping 
                              //                      paClipOff,       // we won't output out of range samples so don't bother clipping them 
                              playCallback,
                              &data );
  */
  PaError err = Pa_OpenDefaultStream(
                              &play_stream,
			      0,
                              2,
			      paFloat32,
                              m_sample_rate,
                              1024,            /* frames per buffer */
                              playCallback,
                              &data );
  if( err != paNoError ) goto error;

  if( play_stream ) {
    err = Pa_StartStream( play_stream );
    if( err != paNoError ) goto error;
  }

  return true;

 error:
  m_err_str = Pa_GetErrorText(err);
  return false;
}

bool playStream::close() {
  return paNoError == Pa_CloseStream( play_stream );
}

void playStream::SetFrequency(const float& frequency) {
  //  assert(frequency >= 0);

  if (frequency >= 0) {
    data.currentFreq = frequency;
  }
}

void playStream::SetVolume(const float& amp) {
  //  assert((0 <= amp) && (amp <= 1));

  if ((0 <= amp) && (amp <= 1)) {
    data.maxAmplitude = amp;
  }
}
