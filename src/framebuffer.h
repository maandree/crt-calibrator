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
#ifndef CRT_CALIBRATOR_H
#define CRT_CALIBRATOR_H


#include <stddef.h>
#include <stdint.h>


/**
 * Framebuffer information
 */
typedef struct framebuffer
{
  /**
   * The file descriptor used to access the framebuffer, 0 if not opened
   */
  int fd;
  
  /**
   * The width of the display in pixels
   */
  uint32_t width;
  
  /**
   * The height of the display in pixels
   */
  uint32_t height;
  
  /**
   * Increment for `mem` to move to next pixel on the line
   */
  uint32_t bytes_per_pixel;
  
  /**
   * Increment for `mem` to move down one line but stay in the same column
   */
  uint32_t line_length;
  
  /**
   * Framebuffer pointer, `MAP_FAILED` (from <sys/mman.h>) if not mapped
   */
  int8_t* mem;
  
} framebuffer_t;


/**
 * Figure out how many framebuffers there are on the system
 * 
 * @return  The number of framebuffers on the system
 */
size_t fb_count(void);

/**
 * Open a framebuffer
 * 
 * @param   index  The index of the framebuffer to open
 * @param   fb     Framevuffer information to fill in
 * @return         Zero on success, -1 on error
 */
int fb_open(size_t index, framebuffer_t* restrict fb);

/**
 * Close a framebuffer
 * 
 * @param  fb  The framebuffer information
 */
void fb_close(framebuffer_t* restrict fb);

/**
 * Construct an sRGB colour in 32-bit XRGB encoding to
 * use when specifying colours
 * 
 * @param   red    The red   component from [0, 255] sRGB
 * @param   green  The green component from [0, 255] sRGB
 * @param   blue   The blue  component from [0, 255] sRGB
 * @return         The colour as one 32-bit integer
 */
uint32_t fb_colour(int red, int green, int blue);

/**
 * Print a filled in rectangle to a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The starting pixel on the X axis for the rectangle
 * @param  y       The starting pixel on the Y axis for the rectangle
 * @param  width   The width of the rectangle, in pixels
 * @param  height  The height of the rectangle, in pixels
 */
void fb_fill_rectangle(framebuffer_t* restrict fb, uint32_t colour,
		       uint32_t x, uint32_t y, uint32_t width, uint32_t height);


#endif

