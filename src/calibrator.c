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
#include "calibrator.h"

#include "framebuffer.h"
#include "drmgamma.h"
#include "gamma.h"

#include <stdio.h>


static const int CONTRAST_BRIGHTNESS_LEVELS[21] =
  {
    0, 17, 27, 38, 48, 59, 70, 82, 94, 106, 119, 131,
    144, 158, 171, 185, 198, 212, 226, 241, 255
  };


int main(int argc __attribute__((unused)), char* argv[])
{
  size_t f, y, x, fn = fb_count();
  for (f = 0; f < fn; f++)
    {
      framebuffer_t fb;
      if (fb_open(f, &fb) < 0)
	goto fail;
      
      for (y = 0; y < 4; y++)
	for (x = 0; x < 21; x++)
	  {
	    int v = CONTRAST_BRIGHTNESS_LEVELS[x];
	    uint32_t colour = fb_colour(v * ((y == 1) | (y == 0)),
					v * ((y == 2) | (y == 0)),
					v * ((y == 3) | (y == 0)));
	    fb_fill_rectangle(&fb, colour,
			      x * fb.width / 21,
			      y * fb.height / 4,
			      (x + 1) * fb.width / 21 - x * fb.width / 21,
			      (y + 1) * fb.height / 4 - y * fb.height / 4);
	  }
      
      fb_close(&fb);
    }
  
  return 0;
 fail:
  perror(*argv);
  return 1;
}

