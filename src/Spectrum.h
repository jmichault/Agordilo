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

/**********************************************************************

  Spectrum.h

  This file has been modified from Audacity's Spectrum.h.

**********************************************************************/

#ifndef __SPECTRUM_H__
#define __SPECTRUM_H__

typedef signed short sampleType;   // From Audacity's WaveTrack.h

#if 0
/*
  This function computes the power (mean square amplitude) as
  a function of frequency, for some block of audio data.

  width: the number of samples
  height: the desired number of frequencies
*/
bool ComputeSpectrum(sampleType * data, int width, int height,
                     double rate, float *out, bool autocorrelation);

int GetSpectrumWindowSize();
#endif

bool Autocorrelation(float mData[],      // In
                     int mDataLen,       // In
                     int mWindowSize,    // In
                     float* processed[], // Out
                     int& mProcessedSize // Out
                     );

float bestPeak(float *mProcessed,  // IN
               int mProcessedSize, // IN
               float mRate        // IN
               );

float bestPeak2(float *mProcessed,  // IN
               int mProcessedSize, // IN
               float mRate        // IN
               );

float Freq2Pitch(float freq);
float Pitch2Freq(float pitch);
char *PitchName(int pitch, bool flats);

#endif
