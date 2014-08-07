/**
 * crt-calibrator – Calibration utility for CRT monitors
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "gamma.h"

#include <math.h>


/**
 * Analyse a gamma ramp
 * 
 * @param  stops       The number of stops in the gamma ramp
 * @param  ramp        The gamma ramp
 * @param  gamma       Output parameter for the gamma
 * @param  contrast    Output parameter for the contrast
 * @param  brightness  Output parameter for the brightness
 */
void gamma_analyse(size_t stops, const uint16_t* restrict ramp, double* restrict gamma,
		   double* restrict contrast, double* restrict brightness)
{
  double min, middle, max;
  *brightness = min = (double)(ramp[0])         / (double)0xFFFF;
  *contrast   = max = (double)(ramp[stops - 1]) / (double)0xFFFF;
  middle            = (double)(ramp[stops / 2]) / (double)0xFFFF;
  
  middle = (middle - min) / (max - min);
  *gamma = log(2.0) / log(middle);
}


/**
 * Generate a gamma ramp
 * 
 * @param  stops       The number of stops in the gamma ramp
 * @param  ramp        Memory area to where to write the gamma ramp
 * @param  gamma       The gamma
 * @param  contrast    The contrast
 * @param  brightness  The brightness
 */
void gamma_generate(size_t stops, uint16_t* restrict ramp, double gamma,
		    double contrast, double brightness)
{
  double diff = contrast - brightness;
  double gamma_ = 1.0 / gamma;
  size_t i;
  int32_t y;
  double y_;
  
  for (i = 0; i < stops; i++)
    {
      y_ = (double)i / (double)stops;
      y_ = pow(y_, gamma_) * diff + brightness;
      y = (int32_t)(y * 0xFFFF);
      if (y < 0x0000)  y = 0x0000;
      if (y > 0xFFFF)  y = 0xFFFF;
      ramp[i] = y;
    }
}

