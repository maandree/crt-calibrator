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

#include "gamma.h"
#include "state.h"

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>



/**
 * Draw bars in different shades of grey, red, green and blue
 * used for calibrating the contrast and brightness
 */
void draw_contrast_brightness(void)
{
  const int CONTRAST_BRIGHTNESS_LEVELS[21] =
    {
      0, 17, 27, 38, 48, 59, 70, 82, 94, 106, 119, 131,
      144, 158, 171, 185, 198, 212, 226, 241, 255
    };
  size_t f;
  uint32_t y, x;
  for (f = 0; f < framebuffer_count; f++)
    {
      framebuffer_t* restrict fb = framebuffers + f;
      for (y = 0; y < 4; y++)
        for (x = 0; x < 21; x++)
          {
            int v = CONTRAST_BRIGHTNESS_LEVELS[x];
            uint32_t colour = fb_colour(v * ((y == 1) | (y == 0)),
                                        v * ((y == 2) | (y == 0)),
                                        v * ((y == 3) | (y == 0)));
            fb_fill_rectangle(fb, colour,
                              x * fb->width / 21,
                              y * fb->height / 4,
                              (x + 1) * fb->width / 21 - x * fb->width / 21,
                              (y + 1) * fb->height / 4 - y * fb->height / 4);
          }
    }
}


/**
 * Draw a seven segment display
 *
 * @param  fb      The framebuffer to draw on
 * @param  colour  The intensity of the least intense colour to use
 * @param  x       The X component of the top left corner of the seven segment display
 * @param  y       The Y component of the top left corner of the seven segment display
 */
void draw_digit(framebuffer_t* restrict fb, int colour, uint32_t x, uint32_t y)
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


/**
 * Manipulate a CRT controllers gamma ramps to display a specific digit
 * for one of the seven segment display on only that CRT controller's
 * monitors
 * 
 * @param  crtc    The CRT controller information
 * @param  colour  The intensity of the least intense colour in the seven segment display
 * @param  value   The valud of the digit to display
 */
void gamma_digit(drm_crtc_t* restrict crtc, int colour, size_t value)
{
#define __  0
  const int DIGITS[11] = { 1  | 2  | 4  | __ | 16 | 32 | 64,  /* (0) */
			   __ | __ | 4  | __ | __ | 32 | __,  /* (1) */
			   1  | __ | 4  | 8  | 16 | __ | 64,  /* (2) */
			   1  | __ | 4  | 8  | __ | 32 | 64,  /* (3) */
			   __ | 2  | 4  | 8  | __ | 32 | __,  /* (4) */
			   1  | 2  | __ | 8  | __ | 32 | 64,  /* (5) */
			   1  | 2  | __ | 8  | 16 | 32 | 64,  /* (6) */
			   1  | __ | 4  | __ | __ | 32 | __,  /* (7) */
			   1  | 2  | 4  | 8  | 16 | 32 | 64,  /* (8) */
			   1  | 2  | 4  | 8  | __ | 32 | 64,  /* (9) */
			   __ | __ | __ | __ | __ | __ | __}; /* not visible */
  int i, digit = DIGITS[value];
  
  for (i = 0; i < 7; i++)
    {
      uint16_t c = (digit & (1 << i)) ? 0xFFFF : 0;
      int j = i + colour;
      crtc->red[j] = crtc->green[j] = crtc->blue[j] = c;
    }
#undef __
}


/**
 * Draw an unique index on each monitor
 * 
 * @return  Zero on success, -1 on error
 */
int draw_id(void)
{
  size_t f, c, id = 0;
  for (f = 0; f < framebuffer_count; f++)
    {
      framebuffer_t* restrict fb = framebuffers + f;
      fb_fill_rectangle(fb, fb_colour(0, 0, 0), 0, 0, fb->width, fb->height);
      draw_digit(fb, 1, 40, 40);
      draw_digit(fb, 8, 180, 40);
    }
  for (c = 0; c < crtc_count; c++)
    {
      drm_crtc_t* restrict crtc = crtcs + c;
      if (drm_get_gamma(crtc) < 0)
	return -1;
      gamma_digit(crtc, 1, id < 10 ? 10 : (id / 10) % 10);
      gamma_digit(crtc, 8,                (id /  1) % 10);
      id++;
      if (drm_set_gamma(crtc) < 0)
	return -1;
    }
  return 0;
}


/**
 * Draw squares used as reference when tweeking the gamma correction
 */
void draw_gamma(void)
{
  size_t f;
  uint32_t x, y;
  for (f = 0; f < framebuffer_count; f++)
    {
      framebuffer_t* restrict fb = framebuffers + f;
      for (x = 0; x < 4; x++)
	{
	  int r = (x == 1) || (x == 0);
	  int g = (x == 2) || (x == 0);
	  int b = (x == 3) || (x == 0);
	  uint32_t background = fb_colour(128 * r, 128 * g, 128 * b);
	  uint32_t average    = fb_colour(188 * r, 188 * g, 188 * b);
	  uint32_t high       = fb_colour(255 * r, 255 * g, 255 * b);
	  uint32_t low        = fb_colour(0, 0, 0);
	  uint32_t xoff = x * fb->width / 4;
	  fb_fill_rectangle(fb, background, xoff, 0, fb->width / 4, fb->height);
	  xoff += (fb->width / 4 - 200) / 2;
	  fb_fill_rectangle(fb, high, xoff, 40, 200, 200);
	  fb_fill_rectangle(fb, average, xoff + 50, 40, 100, 200);
	  fb_fill_rectangle(fb, average, xoff, 280, 200, 200);
	  for (y = 0; y < 200; y += 2)
	    {
	      fb_draw_horizontal_line(fb, high, xoff + 50, 280 + y + 0, 100);
	      fb_draw_horizontal_line(fb, low , xoff + 50, 280 + y + 1, 100);
	    }
	  fb_fill_rectangle(fb, average, xoff, 520, 200, 200);
	  fb_fill_rectangle(fb, high, xoff + 50, 520, 100, 200);
	}
    }
}


/**
 * Print a pattern on the screen that can be used when
 * calibrating the convergence
 */
void draw_convergence(void)
{
  uint32_t black = fb_colour(0, 0, 0);
  uint32_t white = fb_colour(255, 255, 255);
  uint32_t x, y;
  size_t f;
  for (f = 0; f < framebuffer_count; f++)
    {
      framebuffer_t* restrict fb = framebuffers + f;
      fb_fill_rectangle(fb, black, 0, 0, fb->width, fb->height);
      for (y = 0; y <= fb->height; y += 16)
	{
	  if (y == fb->height)
	    y = fb->height - 1;
	  for (x = 0; x <= fb->width; x += 16)
	    {
	      if (x == fb->height)
		x = fb->height - 1;
	      fb_draw_pixel(fb, white, x, y);
	    }
	}
    }
}


/**
 * Print a pattern on the screen that can be used when
 * calibrating the moiré cancellation
 * 
 * @param  gap       The horizontal and vertical gap, in pixels, between the dots
 * @param  diagonal  Whether to draw dots in a diagonal pattern
 */
void draw_moire(uint32_t gap, int diagonal)
{
  uint32_t black = fb_colour(0, 0, 0);
  uint32_t white = fb_colour(255, 255, 255);
  uint32_t x, y, gap2 = gap << 1;
  size_t f;
  gap += (uint32_t)(!diagonal);
  if (diagonal)
    for (f = 0; f < framebuffer_count; f++)
      {
	framebuffer_t* restrict fb = framebuffers + f;
	fb_fill_rectangle(fb, black, 0, 0, fb->width, fb->height);
	for (y = 0; y < fb->height; y += gap)
	  for (x = (y % gap2); x < fb->width; x += gap2)
	    fb_draw_pixel(fb, white, x, y);
      }
  else
    for (f = 0; f < framebuffer_count; f++)
      {
	framebuffer_t* restrict fb = framebuffers + f;
	fb_fill_rectangle(fb, black, 0, 0, fb->width, fb->height);
	for (y = 0; y < fb->height; y += gap)
	  for (x = 0; x < fb->width; x += gap)
	    fb_draw_pixel(fb, white, x, y);
      }
}


/**
 * Analyse the monitors calibrations
 * 
 * @return  Zero on success, -1 on error
 */
int read_calibs(void)
{
  size_t c;
  for (c = 0; c < crtc_count; c++)
    {
      if (drm_get_gamma(crtcs + c) < 0)
	return -1;
      
      gamma_analyse(crtcs[c].gamma_stops, crtcs[c].red, gammas[0] + c,
		    contrasts[0] + c, brightnesses[0] + c);
      gamma_analyse(crtcs[c].gamma_stops, crtcs[c].green, gammas[1] + c,
		    contrasts[1] + c, brightnesses[1] + c);
      gamma_analyse(crtcs[c].gamma_stops, crtcs[c].blue, gammas[2] + c,
		    contrasts[2] + c, brightnesses[2] + c);
    }
  return 0;
}


/**
 * Apply the selected calibrations to the monitors
 * 
 * @return  Zero on success, -1 on error
 */
int apply_calibs(void)
{
  size_t c;
  for (c = 0; c < crtc_count; c++)
    {
      gamma_generate(crtcs[c].gamma_stops, crtcs[c].red, gammas[0][c],
		     contrasts[0][c], brightnesses[0][c]);
      gamma_generate(crtcs[c].gamma_stops, crtcs[c].green, gammas[1][c],
		     contrasts[1][c], brightnesses[1][c]);
      gamma_generate(crtcs[c].gamma_stops, crtcs[c].blue, gammas[2][c],
		     contrasts[2][c], brightnesses[2][c]);
      
      if (drm_set_gamma(crtcs + c) < 0)
	return -1;
    }
  return 0;
}


/**
 * Print calibrations into a file
 * 
 * @param   f  The file
 * @return     Zero on success, -1 on error
 */
int save_calibs(FILE* f)
{
  size_t c;
  for (c = 0; c < crtc_count; c++)
    {
      if (fprintf(f, "# index = %lu\n",
		  c) < 0)
	return -1;
      
      if (fprintf(f, "edid = %s\n",
		  crtcs[c].edid) < 0)
	return -1;
      
      if (fprintf(f, "brightness = %f:%f:%f\n",
		  brightnesses[0][c],
		  brightnesses[1][c],
		  brightnesses[2][c]) < 0)
	return -1;
      
      if (fprintf(f, "contrast = %f:%f:%f\n",
		  contrasts[0][c],
		  contrasts[1][c],
		  contrasts[2][c]) < 0)
	return -1;
      
      if (fprintf(f, "gamma = %f:%f:%f\n\n",
		  gammas[0][c],
		  gammas[1][c],
		  gammas[2][c]) < 0)
	return -1;
    }
  return 0;
}


int main(int argc, char* argv[])
{
  FILE* output_file = stdout;
  int tty_configured = 0, rc = 0, in_fork = 0;
  struct termios saved_stty;
  struct termios stty;
  pid_t pid;
  
  if ((argc > 2) || ((argc == 2) && (argv[1][0] == '-')))
    {
      printf("USAGE: %s [output-file]\n", *argv);
      return 0;
    }
  
  if ((acquire_video()                      < 0) ||
      (tcgetattr(STDIN_FILENO, &saved_stty) < 0) ||
      (tcgetattr(STDIN_FILENO, &stty)       < 0))
    goto fail;
  
  stty.c_lflag &= (tcflag_t)~(ICANON | ECHO);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &stty) < 0)
    goto fail;
  tty_configured = 1;
  
  printf("\033[?25l");
  fflush(stdout);
  
  pid = fork();
  if ((pid != (pid_t)-1) && (pid != 0))
    {
      int status;
    retry:
      if (waitpid(pid, &status, 0) != (pid_t)-1)
	rc = !!status;
      else
	{
	  if (errno != EINTR)
	    perror(*argv);
	  goto retry;
	}
      goto done;
    }
  else if (pid == 0)
    in_fork = 1;
  
  printf("\033[H\033[2J");
  printf("Please deactivate any program that dynamically\n");
  printf("applies filters to your monitors' colours\n");
  printf("and remove any existing filters.\n");
  printf("In doubt, you probably do not have any.\n");
  printf("Do not try to calibrate CRT monitors will\n");
  printf("they are cold.\n");
  printf("\n");
  printf("You will be presented with an image on each\n");
  printf("monitor. Please use the control panel on your\n");
  printf("to calibrate the contrast and brightness of\n");
  printf("each monitor. The contrasts adjusts the\n");
  printf("brightness of bright colours, and the\n");
  printf("brightness adjusts the brightness of dim\n");
  printf("colours. All rectangles are of equals size\n");
  printf("and they should be distrint from eachother.\n");
  printf("There should only be a slight difference\n");
  printf("between the two darkest colours for each\n");
  printf("colour. The grey colour does not need to\n");
  printf("be perfectly grey but should be close.\n");
  printf("The brightness should be as high as\n");
  printf("possible without the first square being\n");
  printf("any other colour than black or the two first\n");
  printf("square being too distinct from eachother. The\n");
  printf("contrast should be as high as possible without\n");
  printf("causing distortion.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  draw_contrast_brightness();
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  printf("An index will be displayed on each monitor.\n");
  printf("It behoves you to memorise them. They will\n");
  printf("be used in the output when descibing the\n");
  printf("calibrations, and is the index of the monitors\n");
  printf("that are used when changing monitor to\n");
  printf("calibrate.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  if ((read_calibs() < 0) || (draw_id() < 0))
    goto fail;
  
  while (getchar() != 10)
    ;
  
  if (apply_calibs() < 0)
    goto fail;
  
  printf("\033[H\033[2J");
  printf("You will not be given the opportunity to.\n");
  printf("calibrate your monitors' brightness and\n");
  printf("contrast using software incase your monitors\n");
  printf("could not be sufficiently calibrated using\n");
  printf("hardware.\n");
  printf("\n");
  printf("<Left> and <right> is used to change which\n");
  printf("monitor to calibrate. <Left> switches to the\n");
  printf("previous monitor (one lower in index) and\n");
  printf("<right> switches to the next monitor (one\n");
  printf("higher in index.)\n");
  printf("<Up> and <down> is used to increase and\n");
  printf("descrease the settings. respectively.\n");
  printf("<Shift+b> is used to switch to changing the.\n");
  printf("monitor's brightness and <shift+c> switches\n");
  printf("to contrast.\n");
  printf("<r> is used to switch to changing the red.\n");
  printf("channel, <g> switches to the green channel,\n");
  printf("<b> switches to the blue channel, and <a>\n");
  printf("is used to switch to change all channels.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  draw_contrast_brightness();
  
  {
    int c, b = 0, at_contrast = 0;
    int red = 1, green = 1, blue = 1;
    size_t mon = 0;
    while ((c = getchar()) != 10)
      {
	if (b)
	  {
	    b = 0;
	    if ((c == 'A') && at_contrast)
	      {
		contrasts[0][mon] += (double)red / 100;
		contrasts[1][mon] += (double)green / 100;
		contrasts[2][mon] += (double)blue / 100;
	      }
	    else if (c == 'A')
	      {
		brightnesses[0][mon] += (double)red / 100;
		brightnesses[1][mon] += (double)green / 100;
		brightnesses[2][mon] += (double)blue / 100;
	      }
	    else if ((c == 'B') && at_contrast)
	      {
		contrasts[0][mon] -= (double)red / 100;
		contrasts[1][mon] -= (double)green / 100;
		contrasts[2][mon] -= (double)blue / 100;
	      }
	    else if (c == 'B')
	      {
		brightnesses[0][mon] -= (double)red / 100;
		brightnesses[1][mon] -= (double)green / 100;
		brightnesses[2][mon] -= (double)blue / 100;
	      }
	    else if (c == 'C')
	      mon = (mon + 1) % crtc_count;
	    else if (c == 'D')
	      mon = (mon == 0 ? crtc_count : mon) - 1;
	    
	    if ((c == 'A') || (c == 'B'))
	      apply_calibs();
	  }
	else if (c == '[')  b = 1;
	else if (c == 'B')  at_contrast = 0;
	else if (c == 'C')  at_contrast = 1;
	else if (c == 'r')  red = 1, green = 0, blue = 0;
	else if (c == 'g')  red = 0, green = 1, blue = 0;
	else if (c == 'b')  red = 0, green = 0, blue = 1;
	else if (c == 'a')  red = 1, green = 1, blue = 1;
      }
  }
  
  printf("\033[H\033[2J");
  printf("You will now be presented with squares used\n");
  printf("to calibrate the gamma correction. There will\n");
  printf("be four stacks: grey, red, green and blue.\n");
  printf("Each stack has three squares: the upper square\n");
  printf("shows the characterics of how the middle square\n");
  printf("will look if the gamma is too high, and the\n");
  printf("lower shows how the middile will look if the\n");
  printf("gamma is too low. The middle square should\n");
  printf("look like it is one single colour if the gamma\n");
  printf("correction is configured correctly. You may\n");
  printf("have to look from a distance or not focus\n");
  printf("your eyes on the squares to compensate for\n");
  printf("the fact that there actually multiple colours\n");
  printf("is the square.\n");
  printf("The grey should look perfectly grey when you\n");
  printf("are done.\n");
  printf("\n");
  printf("<Left> and <right> is used to change which\n");
  printf("monitor to calibrate. <Left> switches to the\n");
  printf("previous monitor (one lower in index) and\n");
  printf("<right> switches to the next monitor (one\n");
  printf("higher in index.)\n");
  printf("<Up> and <down> is used to increase and\n");
  printf("descrease the gamma. respectively.\n");
  printf("<r> is used to switch to changing the red.\n");
  printf("channel, <g> switches to the green channel,\n");
  printf("<b> switches to the blue channel, and <a>\n");
  printf("is used to switch to change all channels.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  draw_gamma();
  
  {
    int c, b = 0;
    int red = 1, green = 1, blue = 1;
    size_t mon = 0;
    while ((c = getchar()) != 10)
      {
	if (b)
	  {
	    b = 0;
	    if (c == 'A')
	      {
		gammas[0][mon] += (double)red / 100;
		gammas[1][mon] += (double)green / 100;
		gammas[2][mon] += (double)blue / 100;
	      }
	    else if (c == 'B')
	      {
		gammas[0][mon] -= (double)red / 100;
		gammas[1][mon] -= (double)green / 100;
		gammas[2][mon] -= (double)blue / 100;
		if (gammas[0][mon] < 0)  gammas[0][mon] = 0;
		if (gammas[1][mon] < 0)  gammas[1][mon] = 0;
		if (gammas[2][mon] < 0)  gammas[2][mon] = 0;
	      }
	    else if (c == 'C')
	      mon = (mon + 1) % crtc_count;
	    else if (c == 'D')
	      mon = (mon == 0 ? crtc_count : mon) - 1;
	    
	    if ((c == 'A') || (c == 'B'))
	      apply_calibs();
	  }
	else if (c == '[')  b = 1;
	else if (c == 'r')  red = 1, green = 0, blue = 0;
	else if (c == 'g')  red = 0, green = 1, blue = 0;
	else if (c == 'b')  red = 0, green = 0, blue = 1;
	else if (c == 'a')  red = 1, green = 1, blue = 1;
      }
  }
  
  printf("\033[H\033[2J");
  printf("The next step is to calibrate the monitors'\n");
  printf("convergence settings using the monitors'\n");
  printf("control panel. White dots will be printed on the\n");
  printf("screens, you should try to get as many as\n");
  printf("possible of them to appear as pure white dots\n");
  printf("rather than dots splitted in the red, green and\n");
  printf("blue dots. On most CRT monitors this is not\n");
  printf("possible to get all corners perfect.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  draw_convergence();
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  printf("The final step is to calbirate the monitors' moiré\n");
  printf("cancellation. This too is done on the using the\n");
  printf("monitors' control panel.\n");
  printf("\n");
  printf("You can use <d> and the arrow keys to change the\n");
  printf("dot-pattern on the screens.\n");
  printf("\n");
  printf("Press ENTER to continue, and ENTER again when\n");
  printf("your are done.\n");
  fflush(stdout);
  
  while (getchar() != 10)
    ;
  
  printf("\033[H\033[2J");
  fflush(stdout);
  draw_moire(1, 1);
  
  {
    int c, b = 0, d = 1;
    uint32_t gap = 1;
    while ((c = getchar()) != 10)
      {
	if (b)
	  {
	    b = 0;
	    if ((c == 'A') || (c == 'C'))
	      draw_moire(++gap, d);
	    else if ((c == 'B') || (c == 'D'))
	      {
		if (--gap == 0)
		  gap = 1;
		draw_moire(gap, d);
	      }
	  }
	else if (c == '[')
	  b = 1;
	else if (c == 'd')
	  draw_moire(gap, d ^= 1);
      }
  }
  
  printf("\033[H\033[2J");
  fflush(stdout);
  
  if (argc == 2)
    {
      output_file = fopen(argv[1], "w");
      if (output_file == NULL)
	goto fail;
    }
  
  if (save_calibs(output_file) < 0)
    goto fail;
  fflush(output_file);
  
  if (argc == 2)
    {
      if (fclose(output_file))
	goto fail;
    }
  
 done:
  if (in_fork == 0)
    {
      release_video();
      if (tty_configured)
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_stty);
      printf("\033[?25h");
      fflush(stdout);
    }
  return rc;
 fail:
  perror(*argv);
  rc = 1;
  goto done;
}

