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
#include "framebuffer.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stropts.h>
#include <linux/fb.h>
#include <errno.h>


/**
 * The psuedodevice pathname pattern used to access a framebuffer
 */
#ifndef FB_DEVICE_PATTERN
# define FB_DEVICE_PATTERN  "/dev/fb%lu"
#endif


/**
 * The number of elements to allocates to a buffer for a framebuffer device pathname
 */
#define FB_DEVICE_MAX_LEN (sizeof(FB_DEVICE_PATTERN) / sizeof(char) + 3 * sizeof(size_t))



/**
 * Figure out how many framebuffers there are on the system
 * 
 * @return  The number of framebuffers on the system
 */
size_t fb_count(void)
{
  char buf[FB_DEVICE_MAX_LEN];
  size_t count = 0;
  
  for (;; count++)
    {
      sprintf(buf, FB_DEVICE_PATTERN, count);
      if (access(buf, F_OK) < 0)
	return count;
    }
}


/**
 * Open a framebuffer
 * 
 * @param   index  The index of the framebuffer to open
 * @param   fb     Framebuffer information to fill in
 * @return         Zero on success, -1 on error
 */
int fb_open(size_t index, framebuffer_t* restrict fb)
{
  char buf[FB_DEVICE_MAX_LEN];
  struct fb_fix_screeninfo fix_info;
  struct fb_var_screeninfo var_info;
  int old_errno;
  
  fb->fd = 0;
  fb->mem = MAP_FAILED;
  
  sprintf(buf, FB_DEVICE_PATTERN, index);
  fb->fd = open(buf, O_RDWR);
  if (fb->fd == 0)
    goto fail;
  
  if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fix_info) ||
      ioctl(fb->fd, FBIOGET_VSCREENINFO, &var_info))
    goto fail;
  
  fb->mem = mmap(NULL, fix_info.smem_len, PROT_WRITE, MAP_SHARED, fb->fd, 0);
  if (fb->mem == MAP_FAILED)
    goto fail;
  
  fb->mem += var_info.xoffset * (var_info.bits_per_pixel / 8);
  fb->mem += var_info.yoffset * fix_info.line_length;
  
  fb->width           = var_info.xres;
  fb->height          = var_info.yres;
  fb->bytes_per_pixel = var_info.bits_per_pixel / 8;
  fb->line_length     = fix_info.line_length;
  
  return 0;
 fail:
  old_errno = errno;
  fb_close(fb);
  errno = old_errno;
  return -1;
}


/**
 * Close a framebuffer
 * 
 * @param  fb  The framebuffer information
 */
void fb_close(framebuffer_t* restrict fb)
{
  if (fb->fd)
    close(fb->fd);
}


/**
 * Construct an sRGB colour in 32-bit XRGB encoding to
 * use when specifying colours
 * 
 * @param   red    The red   component from [0, 255] sRGB
 * @param   green  The green component from [0, 255] sRGB
 * @param   blue   The blue  component from [0, 255] sRGB
 * @return         The colour as one 32-bit integer
 */
uint32_t fb_colour(int red, int green, int blue)
{
  uint32_t rc = 0;
  rc |= red,   rc <<= 8;
  rc |= green, rc <<= 8;
  rc |= blue;
  return rc;
}


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
		       uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  int8_t* mem = fb->mem + y * fb->line_length;
  size_t x1 = x * fb->bytes_per_pixel;
  size_t x2 = (x + width) * fb->bytes_per_pixel;
  size_t y2 = y + height;
  size_t x_, y_;
  
  for (y_ = y; y_ != y2; y_++, mem += fb->line_length)
    for (x_ = x1; x_ != x2; x_ += fb->bytes_per_pixel)
      {
	int8_t* pixel = mem + x_;
	*(uint32_t*)pixel = colour;
      }
}


/**
 * Draw a horizontal line segment on a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The starting pixel on the X axis for the line segment
 * @param  y       The starting pixel on the Y axis for the line segment
 * @param  length  The length of the line segment, in pixels
 */
void fb_draw_horizontal_line(framebuffer_t* restrict fb, uint32_t colour,
			     uint32_t x, uint32_t y, uint32_t length)
{
  int8_t* mem = fb->mem + y * fb->line_length;
  size_t x1 = x * fb->bytes_per_pixel;
  size_t x2 = (x + length) * fb->bytes_per_pixel;
  size_t x_;
  
  for (x_ = x1; x_ != x2; x_ += fb->bytes_per_pixel)
    {
      int8_t* pixel = mem + x_;
      *(uint32_t*)pixel = colour;
    }
}


/**
 * Draw a vertical line segment on a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The starting pixel on the X axis for the line segment
 * @param  y       The starting pixel on the Y axis for the line segment
 * @param  length  The length of the line segment, in pixels
 */
void fb_draw_vertical_line(framebuffer_t* restrict fb, uint32_t colour,
			   uint32_t x, uint32_t y, uint32_t length)
{
  int8_t* mem = fb->mem + y * fb->line_length + x * fb->bytes_per_pixel;
  size_t y2 = y + length;
  size_t y_;
  
  for (y_ = y; y_ != y2; y_++, mem += fb->line_length)
    *(uint32_t*)mem = colour;
}

