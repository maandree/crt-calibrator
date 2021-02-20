/* See LICENSE file for copyright and license details. */
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>


/**
 * Framebuffer information
 */
typedef struct framebuffer
{
	/**
	 * The file descriptor used to access the framebuffer, -1 if not opened
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
	int8_t *mem;

} framebuffer_t;


/**
 * Graphics card information
 */
typedef struct drm_card
{
	/**
	 * File descriptor for the connection to the graphics card,
	 * -1 if not opened
	 */
	int fd;

	/**
	 * The graphics card's mode resources, `NULL` if not acquired
	 */
	drmModeRes *restrict res;

	/**
	 * The number of CRTC:s available on the graphics card
	 */
	size_t crtc_count;

	/**
	 * The available connectors
	 */
	drmModeConnector **restrict connectors;

	/**
	 * The available encoders
	 */
	drmModeEncoder **restrict encoders;

	/**
	 * The number of connectors and encoders
	 */
	size_t connector_count;

} drm_card_t;


/**
 * CRT controller information
 */
typedef struct drm_crtc
{
	/**
	 * CRT controller identifier
	 */
	uint32_t id;

	/**
	 * The graphics card
	 */
	drm_card_t *restrict card;

	/**
	 * The CRT controller's connector
	 */
	drmModeConnector *restrict connector;

	/**
	 * The CRT controller's encoder
	 */
	drmModeEncoder *restrict encoder;

	/**
	 * Whether the connector is connected
	 */
	int connected;

	/**
	 * The CRT's EDID, hexadecimally encoded
	 */
	char *restrict edid;

	/**
	 * The number of stops on the gamma ramps
	 */
	size_t gamma_stops;

	/**
	 * The gamma ramp for the red channel
	 */
	uint16_t *restrict red;

	/**
	 * The gamma ramp for the green channel
	 */
	uint16_t *restrict green;

	/**
	 * The gamma ramp for the blue channel
	 */
	uint16_t *restrict blue;

} drm_crtc_t;



/***** gamma.c *****/

/**
 * Analyse a gamma ramp
 * 
 * @param  stops       The number of stops in the gamma ramp
 * @param  ramp        The gamma ramp
 * @param  gamma       Output parameter for the gamma
 * @param  contrast    Output parameter for the contrast
 * @param  brightness  Output parameter for the brightness
 */
void gamma_analyse(size_t stops, const uint16_t* restrict ramp, double *restrict gamma,
                   double *restrict contrast, double* restrict brightness);

/**
 * Generate a gamma ramp
 * 
 * @param  stops       The number of stops in the gamma ramp
 * @param  ramp        Memory area to where to write the gamma ramp
 * @param  gamma       The gamma
 * @param  contrast    The contrast
 * @param  brightness  The brightness
 */
void gamma_generate(size_t stops, uint16_t *restrict ramp, double gamma, double contrast, double brightness);



/***** framebuffer.c *****/

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
 * @param   fb     Framebuffer information to fill in
 * @return         Zero on success, -1 on error
 */
int fb_open(size_t index, framebuffer_t *restrict fb);

/**
 * Close a framebuffer
 * 
 * @param  fb  The framebuffer information
 */
void fb_close(framebuffer_t *restrict fb);

/**
 * Construct an sRGB colour in 32-bit XRGB encoding to
 * use when specifying colours
 * 
 * @param   red    The red   component from [0, 255] sRGB
 * @param   green  The green component from [0, 255] sRGB
 * @param   blue   The blue  component from [0, 255] sRGB
 * @return         The colour as one 32-bit integer
 */
#ifdef __GNUC__
__attribute__((__const__))
#endif
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
void fb_fill_rectangle(framebuffer_t *restrict fb, uint32_t colour, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/**
 * Draw a horizontal line segment on a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The starting pixel on the X axis for the line segment
 * @param  y       The starting pixel on the Y axis for the line segment
 * @param  length  The length of the line segment, in pixels
 */
void fb_draw_horizontal_line(framebuffer_t *restrict fb, uint32_t colour, uint32_t x, uint32_t y, uint32_t length);

/**
 * Draw a vertical line segment on a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The starting pixel on the X axis for the line segment
 * @param  y       The starting pixel on the Y axis for the line segment
 * @param  length  The length of the line segment, in pixels
 */
void fb_draw_vertical_line(framebuffer_t *restrict fb, uint32_t colour, uint32_t x, uint32_t y, uint32_t length);

/**
 * Draw a single on a framebuffer
 * 
 * @param  fb      The framebuffer
 * @param  colour  The colour to use when drawing the rectangle
 * @param  x       The pixel's position on the X axis
 * @param  y       The pixel's position on the Y axis
 */
static inline void
fb_draw_pixel(framebuffer_t *restrict fb, uint32_t colour, uint32_t x, uint32_t y)
{
	int8_t *mem = fb->mem + y * fb->line_length + x * fb->bytes_per_pixel;
	*(uint32_t *)mem = colour;
}



/***** state.c ******/

/**
 * The framebuffers on the system
 */
extern framebuffer_t *restrict framebuffers;

/**
 * The number of elements in `framebuffers`
 */
extern size_t framebuffer_count;

/**
 * The graphics cards on the system
 */
extern drm_card_t *restrict cards;

/**
 * The number of elements in `cards`
 */
extern size_t card_count;

/**
 * The connected CRT controllers on the system
 */
extern drm_crtc_t *restrict crtcs;

/**
 * The software brightness setting on each connected CRT controller, on each channel
 */
extern double *restrict brightnesses[3];

/**
 * The software contrast setting on each connected CRT controller, on each channel
 */
extern double *restrict contrasts[3];

/**
 * The gamma correction on each connected CRT controller, on each channel
 */
extern double *restrict gammas[3];

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



/***** drmgamma.c ******/

/**
 * Figure out how many graphics cards there are on the system
 * 
 * @return  The number of graphics cards on the system
 */
size_t drm_card_count(void);

/**
 * Acquire access to a graphics card
 * 
 * @param   index  The index of the graphics card
 * @param   card   Graphics card information to fill in
 * @return         Zero on success, -1 on error
 */
int drm_card_open(size_t index, drm_card_t *restrict card);

/**
 * Release access to a graphics card
 * 
 * @param  card  The graphics card information
 */
void drm_card_close(drm_card_t *restrict card);

/**
 * Acquire access to a CRT controller
 * 
 * @param   index  The index of the CRT controller
 * @param   card   The graphics card information
 * @param   crtc   CRT controller information to fill in
 * @return         Zero on success, -1 on error
 */
int drm_crtc_open(size_t index, drm_card_t *restrict card, drm_crtc_t *restrict crtc);

/**
 * Release access to a CRT controller
 * 
 * @param  crtc  The CRT controller information to fill in
 */
void drm_crtc_close(drm_crtc_t *restrict crtc);

/**
 * Read the gamma ramps for a CRT controller
 * 
 * @param   crtc  CRT controller information
 * @return        Zero on success, -1 on error 
 */
int drm_get_gamma(drm_crtc_t *restrict crtc);

/**
 * Apply gamma ramps for a CRT controller
 * 
 * @param   crtc  CRT controller information
 * @return        Zero on success, -1 on error
 */
int drm_set_gamma(drm_crtc_t *restrict crtc);
