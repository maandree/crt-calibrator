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
#ifndef CRT_CALIBRATOR_CALIBRATOR_H
#define CRT_CALIBRATOR_CALIBRATOR_H


#include "framebuffer.h"
#include "drmgamma.h"

#include <stdint.h>
#include <stddef.h>


/**
 * Draw bars in different shades of grey, red, green and blue
 * used for calibrating the contrast and brightness
 */
void draw_contrast_brightness(void);

/**
 * Draw a seven segment display
 * 
 * @param  fb      The framebuffer to draw on
 * @param  colour  The intensity of the least intense colour to use
 * @param  x       The X component of the top left corner of the seven segment display
 * @param  y       The Y component of the top left corner of the seven segment display
 */
void draw_digit(framebuffer_t* restrict fb, int colour, uint32_t x, uint32_t y);

/**
 * Manipulate a CRT controllers gamma ramps to display a specific digit
 * for one of the seven segment display on only that CRT controller's
 * monitors
 * 
 * @param  crtc    The CRT controller information
 * @param  colour  The intensity of the least intense colour in the seven segment display
 * @param  value   The valud of the digit to display
 */
void gamma_digit(drm_crtc_t* restrict crtc, int colour, size_t value);

/**
 * Draw an unique index on each monitor
 * 
 * @return  Zero on success, -1 on error
 */
int draw_id(void);

/**
 * Draw squares used as reference when tweeking the gamma correction
 */
void draw_gamma(void);

/**
 * Print a pattern on the screen that can be used when
 * calibrating the convergence
 */
void draw_convergence(void);

/**
 * Print a pattern on the screen that can be used when
 * calibrating the moiré cancellation
 * 
 * @param  gap       The horizontal and vertical gap, in pixels, between the dots
 * @param  diagonal  Whether to draw dots in a diagonal pattern
 */
void draw_moire(uint32_t gap, int diagonal);

/**
 * Analyse the monitors calibrations
 * 
 * @return  Zero on success, -1 on error
 */
int read_calibs(void);

/**
 * Apply the selected calibrations to the monitors
 * 
 * @return  Zero on success, -1 on error
 */
int apply_calibs(void);

/**
 * Print calibrations into a file
 * 
 * @param   f  The file
 * @return     Zero on success, -1 on error
 */
int save_calibs(FILE* f);


#endif

