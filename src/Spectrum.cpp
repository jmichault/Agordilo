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

  Spectrum.cpp

  This file has been modified from Audacity's Spectrum.cpp.
  Actually, the code in use is from Audacity's FreqWindow.cpp.

**********************************************************************/

#include <math.h>
#include <stdio.h>    // For sprintf()

#include "Spectrum.h"
#include "FFT.h"

// This is Audacity's FreqWindow::Recalc(), but shaved down to 
//   1) be enhanced auto-correlation only
//   2) take parameters, and return values.
bool Autocorrelation(float mData[],      // In
                     int mDataLen,       // In
                     int mWindowSize,    // In
                     float* processed[], // Out
                     int& mProcessedSize // Out
                     )
{
  if (mDataLen < mWindowSize) {
    // Not enough data to get even one window
    return false;
  }

//   float *mProcessed = NULL;
   float *mProcessed = *processed;

//   mProcessed = new float[mWindowSize];

   int i;
   for (i = 0; i < mWindowSize; i++)
      mProcessed[i] = 0.0;
   int half = mWindowSize / 2;

   float *in = new float[mWindowSize];
   float *in2 = new float[mWindowSize];
   float *out = new float[mWindowSize];
   float *out2 = new float[mWindowSize];

   int start = 0;
   int windows = 0;
   while (start + mWindowSize <= mDataLen) {
      // Copy stuff into in
      for (i = 0; i < mWindowSize; i++)
         in[i] = mData[start + i];

      // Window the data to lose crazy artifacts
      // due to finite-length window
      WindowFunc(2 /* Hamming */, mWindowSize, in);

      // Enhanced AC

      // Take FFT
      FFT(mWindowSize, false, in, NULL, out, out2);

      // Compute power
      for (i = 0; i < mWindowSize; i++)
        in[i] = (out[i] * out[i]) + (out2[i] * out2[i]);

      // Tolonen and Karjalainen recommend taking the cube root
      // of the power, instead of the square root

      for (i = 0; i < mWindowSize; i++)
        in[i] = pow(in[i], 1.0 / 3.0);

      // Take FFT
      FFT(mWindowSize, false, in, NULL, out, out2);

      // Take real part of result
      for (i = 0; i < half; i++)
        mProcessed[i] += out[i];

      start += half;
      windows++;
   }

   // Enhanced Autocorrelation
   for (i = 0; i < half; i++)
     mProcessed[i] = mProcessed[i] / windows;

   // Peak Pruning as described by Tolonen and Karjalainen, 2000

   // Clip at zero, copy to temp array
   for (i = 0; i < half; i++) {
     if (mProcessed[i] < 0.0)
       mProcessed[i] = 0.0;
     out[i] = mProcessed[i];
   }

   // Subtract a time-doubled signal (linearly interp.) from the original
   // (clipped) signal
   for (i = 0; i < half; i++)
     if ((i % 2) == 0)
       mProcessed[i] -= out[i / 2];
     else
       mProcessed[i] -= ((out[i / 2] + out[i / 2 + 1]) / 2);

   // Clip at zero again
   for (i = 0; i < half; i++)
     if (mProcessed[i] < 0.0)
       mProcessed[i] = 0.0;

   mProcessedSize = half;

   delete[]in;
   delete[]in2;
   delete[]out;
   delete[]out2;

//   *processed = mProcessed;

   return true;
}

float Freq2Pitch(float freq)
{
   return float (69.0 + 12.0 * (log(freq / 440.0) / log(2.0)));
}

float Pitch2Freq(float pitch)
{
  return 440.0 * pow(2, (pitch - 69.0) / 12.0);
}

char gPitchName[10];

char *PitchName(int pitch, bool flats)
{
   char *p = gPitchName;

   switch (pitch % 12) {
   case 0:
      *p++ = 'C';
      break;
   case 1:
      if (flats) {
         *p++ = 'D';
         *p++ = 'b';
      } else {
         *p++ = 'C';
         *p++ = '#';
      }
      break;
   case 2:
      *p++ = 'D';
      break;
   case 3:
      if (flats) {
         *p++ = 'E';
         *p++ = 'b';
      } else {
         *p++ = 'D';
         *p++ = '#';
      }
      break;
   case 4:
      *p++ = 'E';
      break;
   case 5:
      *p++ = 'F';
      break;
   case 6:
      if (flats) {
         *p++ = 'G';
         *p++ = 'b';
      } else {
         *p++ = 'F';
         *p++ = '#';
      }
      break;
   case 7:
      *p++ = 'G';
      break;
   case 8:
      if (flats) {
         *p++ = 'A';
         *p++ = 'b';
      } else {
         *p++ = 'G';
         *p++ = '#';
      }
      break;
   case 9:
      *p++ = 'A';
      break;
   case 10:
      if (flats) {
         *p++ = 'B';
         *p++ = 'b';
      } else {
         *p++ = 'A';
         *p++ = '#';
      }
      break;
   case 11:
      *p++ = 'B';
      break;
   }

//   sprintf(p, "%d", ((pitch + 3) / 12) - 2);
   sprintf(p, "%d", ((pitch ) / 12) - 1);

   return gPitchName;
}


float CubicMaximize(float y0, float y1, float y2, float y3, float *maxyVal)
{
   // Find coefficients of cubic
   float a, b, c, d;

   a = y0 / -6.0 + y1 / 2.0 - y2 / 2.0 + y3 / 6.0;
   b = y0 - 5.0 * y1 / 2.0 + 2.0 * y2 - y3 / 2.0;
   c = -11.0 * y0 / 6.0 + 3.0 * y1 - 3.0 * y2 / 2.0 + y3 / 3.0;
   d = y0;

   // Take derivative
   float da, db, dc;

   da = 3 * a;
   db = 2 * b;
   dc = c;

   // Find zeroes of derivative using quadratic equation
   float discriminant = db * db - 4 * da * dc;
   if (discriminant < 0.0)
      return -1.0;              // error

   float x1 = (-db + sqrt(discriminant)) / (2 * da);
   float x2 = (-db - sqrt(discriminant)) / (2 * da);

   // The one which corresponds to a local _maximum_ in the
   // cubic is the one we want - the one with a negative
   // second derivative
   float dda = 2 * da;
   float ddb = db;

#define CUBIC(x,a,b,c,d)  (a*x*x*x + b*x*x + c*x + d)

   if (dda * x1 + ddb < 0) {
     *maxyVal = CUBIC(x1,a,b,c,d);
     return x1;
   }
   else {
     *maxyVal = CUBIC(x2,a,b,c,d);
     return x2;
   }

#undef CUBIC
}

float bestPeak(float *mProcessed,  // IN
               int mProcessedSize, // IN
               float mRate         // IN
               ) {
   float highestpeak_y = 0;
   float highestpeak_x = 0;

   bool up = (mProcessed[1] > mProcessed[0]);
   for (int bin = 2; bin < mProcessedSize; bin++) {
     bool nowUp = mProcessed[bin] > mProcessed[bin - 1];
     if (!nowUp && up) {
       // Local maximum.  Find actual value by cubic interpolation
       //printf("Local peak at bin %d\n", bin);
       int leftbin = bin - 2;
       if (leftbin < 1)
         leftbin = 1;

       float thispeak_y;
       float max = leftbin + CubicMaximize(mProcessed[leftbin],
                                           mProcessed[leftbin + 1],
                                           mProcessed[leftbin + 2],
                                           mProcessed[leftbin + 3],
                                           &thispeak_y);

       float thispeak_x = max;

       if (thispeak_y > highestpeak_y) {
         highestpeak_x = thispeak_x;
         highestpeak_y = thispeak_y;
       }
     }
     up = nowUp;
   }

   return highestpeak_x;
}


float Parabole(float *y, int nb, float *maxyVal)
{
 int i;
 float mx4=0,mx3=0,mx2=0,mx=0;
 float mx3y=0,mx2y=0,mxy=0,my=0;
 float a,b,c;
 float highX;
  for (i=0 ; i < nb ; i++)
  {
    mx  += i;
    mx2 += i*i;
    mx3 += i*i*i;
    mx4 += i*i*i*i;
    mxy  += i*y[i];
    mx2y += i*i*y[i];
    mx3y += i*i*i*y[i];
    my += y[i];
  }
  mx /= nb;
  mx2 /= nb;
  mx3 /= nb;
  mx4 /= nb;
  my /= nb;
  mxy /= nb;
  mx2y /= nb;
  mx3y /= nb;
  a=  ((mx2y-mx2*my)*(mx2-mx*mx)-(mxy-mx*my)*(mx3-mx2*mx))
     /((mx4-mx2*mx2)*(mx2-mx*mx)-(mx3-mx*mx2)*(mx3-mx*mx2));
  b=  (mxy-mx*my-a*(mx3-mx*mx2))/(mx2-mx*mx);
  c = my-a*mx2-b*mx;
  highX = (-b/(2*a));
  *maxyVal = a*highX*highX+b*highX+c;
  return (-b/(2*a));
}

float bestPeak2(float *mProcessed,  // IN
               int mProcessedSize, // IN
               float mRate         // IN
               ) {
   float highestpeak_y = 0;
   float highestpeak_x = 0;
   int iMaxX=0;
   int bin;

   bool up = (mProcessed[1] > mProcessed[0]);
   for ( bin = 2; bin < mProcessedSize; bin++) {
     bool nowUp = mProcessed[bin] > mProcessed[bin - 1];
     if (!nowUp && up) {
       if (mProcessed[bin-1] > highestpeak_y) {
         highestpeak_y = mProcessed[bin-1];
	 iMaxX = bin-1;
       }
     }
     up = nowUp;
   }
   // cherche le pic par recherche de la parabole la plus proche.
   int leftbin = iMaxX-1;
   while (    leftbin>1
	   && leftbin >(iMaxX-20)
	   && mProcessed[leftbin-1]>mProcessed[iMaxX]/2
	   )
      leftbin--;
   int nb = (iMaxX-leftbin)*2+1;
   float thispeak_y;
   float max = leftbin + Parabole(&mProcessed[leftbin],nb,&thispeak_y);
   return max;
}
