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


static int draw_contrast_brightness(void)
{
  size_t f, y, x, fn = fb_count();
  for (f = 0; f < fn; f++)
    {
      framebuffer_t fb;
      if (fb_open(f, &fb) < 0)
        return -1;
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
}


static void draw_digit(framebuffer_t* restrict fb, int colour, uint32_t x, uint32_t y)
{
  uint32_t c;
  
  c = fb_colour(colour + 0, colour + 0, colour + 0);
  fb_fill_rectangle(fb, c, x + 20, y, 80, 20);
  
  c = fb_colour(colour + 1, colour + 1, colour + 1);
  fb_fill_rectangle(fb, c, x, y + 20, 20, 80);
  
  c = fb_colour(colour + 2, colour + 2, colour + 2);
  fb_fill_rectangle(fb, c, x + 100, y + 20, 20, 80);
  
  c = fb_colour(colour + 3, colour + 3, colour + 3);
  fb_fill_rectangle(fb, c, x + 20, y + 100, 80, 20);
  
  c = fb_colour(colour + 4, colour + 4, colour + 4);
  fb_fill_rectangle(fb, c, x, y + 120, 20, 80);
  
  c = fb_colour(colour + 5, colour + 5, colour + 5);
  fb_fill_rectangle(fb, c, x + 100, y + 120, 20, 80);
  
  c = fb_colour(colour + 6, colour + 6, colour + 6);
  fb_fill_rectangle(fb, c, x + 20, y + 200, 80, 20);
}


static int gamma_digit(drm_crtc_t* restrict crtc, int colour, size_t value)
{
#define _  0
  const int DIGITS[11] = { 1 | 2 | 4 | _ | 16 | 32 | 64,  /* (0) */
			   _ | _ | 4 | _ | _  | 32 | _,   /* (1) */
			   1 | _ | 4 | 8 | 16 | _  | 64,  /* (2) */
			   1 | _ | 4 | 8 | _  | 32 | 64,  /* (3) */
			   _ | 2 | 4 | 8 | _  | 32 | _,   /* (4) */
			   1 | 2 | _ | 8 | _  | 32 | 64,  /* (5) */
			   1 | 2 | _ | 8 | 16 | 32 | 64,  /* (6) */
			   1 | _ | 4 | _ | _  | 32 | _,   /* (7) */
			   1 | 2 | 4 | 8 | 16 | 32 | 64,  /* (8) */
			   1 | 2 | 4 | 8 | _  | 32 | 64,  /* (9) */
			   _ | _ | _ | _ | _  | _  | _}; /* not visible */
  int i, digit = DIGITS[value];
  
  for (i = 0; i < 7; i++)
    {
      uint16_t value = (digit & (1 << i)) ? 0xFFFF : 0;
      int j = i + colour;
      crtc->red[j] = crtc->green[j] = crtc->blue[j] = value;
    }
#undef _
}


static int draw_id(void)
{
  size_t f, c, i, id = 0, fn = fb_count(), cn = drm_card_count();
  for (f = 0; f < fn; f++)
    {
      framebuffer_t fb;
      if (fb_open(f, &fb) < 0)
	return -1;
      fb_fill_rectangle(&fb, fb_colour(0, 0, 0), 0, 0, fb.width, fb.height);
      draw_digit(&fb, 1, 40, 40);
      draw_digit(&fb, 8, 180, 40);
      fb_close(&fb);
    }
  for (c = 0; c < cn; c++)
    {
      drm_card_t card;
      if (drm_card_open(c, &card) < 0)
	return -1;
      for (i = 0; i < card.crtc_count; i++)
	{
	  drm_crtc_t crtc;
	  if (drm_crtc_open(i, &card, &crtc) < 0)
	    {
	      drm_card_close(&card);
	      return -1;
	    }
	  if (crtc.connected == 0)
	    goto not_connected;
	  if (drm_get_gamma(&crtc) < 0)
	    goto crtc_fail;
	  gamma_digit(&crtc, 1, id < 10 ? 10 : (id / 10) % 10);
	  gamma_digit(&crtc, 8,                (id /  1) % 10);
	  id++;
	  if (drm_set_gamma(&crtc) < 0)
	    goto crtc_fail;
	not_connected:
	  drm_crtc_close(&crtc);
	  
	  continue;
	crtc_fail:
	  drm_crtc_close(&crtc);
	  drm_card_close(&card);
	  return -1;
	}
      drm_card_close(&card);
    }
  return 0;
}


int main(int argc __attribute__((unused)), char* argv[])
{
  if (draw_id() < 0)
    goto fail;
  
  return 0;
 fail:
  perror(*argv);
  return 1;
}

