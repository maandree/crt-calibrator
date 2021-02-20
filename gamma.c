/* See LICENSE file for copyright and license details. */
#include "common.h"


/**
 * Analyse a gamma ramp
 * 
 * @param  stops       The number of stops in the gamma ramp
 * @param  ramp        The gamma ramp
 * @param  gamma       Output parameter for the gamma
 * @param  contrast    Output parameter for the contrast
 * @param  brightness  Output parameter for the brightness
 */
void gamma_analyse(size_t stops, const uint16_t *restrict ramp, double *restrict gamma,
                   double *restrict contrast, double *restrict brightness)
{
	double min, middle, max;
	*brightness = min = (double)(ramp[0])         / (double)0xFFFF;
	*contrast   = max = (double)(ramp[stops - 1]) / (double)0xFFFF;
	middle            = (double)(ramp[stops / 2]) / (double)0xFFFF;

	if (!(stops % 2)) {
		middle += (double)(ramp[stops / 2 - 1]) / (double)0xFFFF;
		middle /= (double)2;
	}

	middle = (middle - min) / (max - min);
	*gamma = -log((double)2) / log(middle);
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
void gamma_generate(size_t stops, uint16_t *restrict ramp, double gamma, double contrast, double brightness)
{
	double diff = contrast - brightness;
	double gamma_ = (double)1 / gamma;
	size_t i;
	int32_t y;
	double y_;

	for (i = 0; i < stops; i++) {
		y_ = (double)i / (double)stops;
		y_ = pow(y_, gamma_) * diff + brightness;
		y = (int32_t)(y_ * 0xFFFF);
		if (y < 0x0000)  y = 0x0000;
		if (y > 0xFFFF)  y = 0xFFFF;
		ramp[i] = (uint16_t)y;
	}
}
