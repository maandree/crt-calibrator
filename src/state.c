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
#include "state.h"

#include <stdlib.h>


/**
 * The framebuffers on the system
 */
framebuffer_t* restrict framebuffers = NULL;

/**
 * The number of elements in `framebuffers`
 */
size_t framebuffer_count = 0;

/**
 * The graphics cards on the system
 */
drm_card_t* restrict cards = NULL;

/**
 * The number of elements in `cards`
 */
size_t card_count = 0;

/**
 * The connected CRT controllers on the system
 */
drm_crtc_t* restrict crtcs = NULL;

/**
 * The number of elements in `crtcs`
 */
size_t crtc_count = 0;



/**
 * Acquire video control
 * 
 * @return  Zero on success, -1 on error
 */
int acquire_video(void)
{
  size_t f, c, i, fn = fb_count(), cn = drm_card_count();
  drm_crtc_t* restrict old_crtcs;
  
  framebuffers = malloc(fn * sizeof(framebuffer_t));
  if (framebuffers == NULL)
    return -1;
  
  for (f = 0; f < fn; f++)
    {
      framebuffer_t fb;
      if (fb_open(f, &fb) < 0)
	return -1;
      framebuffers[framebuffer_count++] = fb;
    }
  
  cards = malloc(cn * sizeof(drm_card_t));
  if (cards == NULL)
    return -1;
  
  for (c = 0; c < cn; c++)
    {
      drm_card_t card;
      if (drm_card_open(c, &card) < 0)
	return -1;
      cards[card_count++] = card;
      
      old_crtcs = crtcs;
      crtcs = realloc(crtcs, (crtc_count + card.crtc_count) * sizeof(drm_crtc_t));
      if (crtcs == NULL)
	{
	  crtcs = old_crtcs;
	  return -1;
	}
      
      for (i = 0; i < card.crtc_count; i++)
	{
	  drm_crtc_t crtc;
	  if (drm_crtc_open(i, cards + c, &crtc) < 0)
	    return -1;
	  if (crtc.connected)
	    crtcs[crtc_count++] = crtc;
	  else
	    drm_crtc_close(&crtc);
	}
    }
  
  return 0;
}


/**
 * Release video control
 */
void release_video(void)
{
  size_t i;
  
  for (i = 0; i < crtc_count; i++)
    drm_crtc_close(crtcs + i);
  crtc_count = 0;
  
  for (i = 0; i < card_count; i++)
    drm_card_close(cards + i);
  card_count = 0;
  
  for (i = 0; i < framebuffer_count; i++)
    fb_close(framebuffers + i);
  framebuffer_count = 0;
  
  free(crtcs), crtcs = NULL;
  free(cards), cards = NULL;
  free(framebuffers), framebuffers = NULL;
}

