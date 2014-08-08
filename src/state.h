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
#ifndef CRT_CALIBRATOR_STATE_H
#define CRT_CALIBRATOR_STATE_H


#include "framebuffer.h"
#include "drmgamma.h"

#include <stddef.h>


/**
 * The framebuffers on the system
 */
extern framebuffer_t* restrict framebuffers;

/**
 * The number of elements in `framebuffers`
 */
extern size_t framebuffer_count;

/**
 * The graphics cards on the system
 */
extern drm_card_t* restrict cards;

/**
 * The number of elements in `cards`
 */
extern size_t card_count;

/**
 * The connected CRT controllers on the system
 */
extern drm_crtc_t* restrict crtcs;

/**
 * The software brightness setting on each connected CRT controller, on each channel
 */
extern double* restrict brightnesses[3];

/**
 * The software contrast setting on each connected CRT controller, on each channel
 */
extern double* restrict contrasts[3];

/**
 * The gamma correction on each connected CRT controller, on each channel
 */
extern double* restrict gammas[3];

/**
 * The number of elements in `crtcs`, `brightnesses[]`, `contrasts[]` and `gammas[]`
 */
extern size_t crtc_count;



/**
 * Acquire video control
 * 
 * @return  Zero on success, -1 on error
 */
int acquire_video(void);

/**
 * Release video control
 */
void release_video(void);


#endif

