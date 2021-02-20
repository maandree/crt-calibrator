/* See LICENSE file for copyright and license details. */
#include "common.h"


/**
 * The framebuffers on the system
 */
framebuffer_t *restrict framebuffers = NULL;

/**
 * The number of elements in `framebuffers`
 */
size_t framebuffer_count = 0;

/**
 * The graphics cards on the system
 */
drm_card_t *restrict cards = NULL;

/**
 * The number of elements in `cards`
 */
size_t card_count = 0;

/**
 * The connected CRT controllers on the system
 */
drm_crtc_t *restrict crtcs = NULL;

/**
 * The software brightness setting on each connected CRT controller, on each channel
 */
double *restrict brightnesses[3];

/**
 * The software contrast setting on each connected CRT controller, on each channel
 */
double *restrict contrasts[3];

/**
 * The gamma correction on each connected CRT controller, on each channel
 */
double *restrict gammas[3];

/**
 * The number of elements in `crtcs`, `brightnesses[]`, `contrasts[]` and `gammas[]`
 */
size_t crtc_count = 0;



/**
 * Acquire video control
 * 
 * @return  Zero on success, -1 on error
 */
int
acquire_video(void)
{
	size_t f, c, i, fn = fb_count(), cn = drm_card_count();
	drm_crtc_t *restrict old_crtcs, crtc;
	drm_card_t card;
	framebuffer_t fb;

	framebuffers = malloc(fn * sizeof(framebuffer_t));
	if (!framebuffers)
		return -1;

	for (f = 0; f < fn; f++) {
		if (fb_open(f, &fb) < 0)
			return -1;
		framebuffers[framebuffer_count++] = fb;
	}

	cards = malloc(cn * sizeof(drm_card_t));
	if (!cards)
		return -1;

	for (c = 0; c < cn; c++) {
		if (drm_card_open(c, &card) < 0)
			return -1;
		cards[card_count++] = card;

		old_crtcs = crtcs;
		crtcs = realloc(crtcs, (crtc_count + card.crtc_count) * sizeof(drm_crtc_t));
		if (!crtcs) {
			crtcs = old_crtcs;
			return -1;
		}

		for (i = 0; i < card.crtc_count; i++) {
			if (drm_crtc_open(i, cards + c, &crtc) < 0)
				return -1;
			if (crtc.connected)
				crtcs[crtc_count++] = crtc;
			else
				drm_crtc_close(&crtc);
		}
	}

	for (c = 0; c < 3; c++) {
		brightnesses[c] = malloc(crtc_count * sizeof(double));
		if (!brightnesses[c])
			return -1;
		contrasts[c] = malloc(crtc_count * sizeof(double));
		if (!contrasts[c])
			return -1;
		gammas[c] = malloc(crtc_count * sizeof(double));
		if (!gammas[c])
			return -1;
	}

	return 0;
}


/**
 * Release video control
 */
void
release_video(void)
{
	size_t i;
	while (crtc_count)
		drm_crtc_close(&crtcs[--crtc_count]);
	while (card_count)
		drm_card_close(&cards[--card_count]);
	while (framebuffer_count)
		fb_close(&framebuffers[--framebuffer_count]);
	for (i = 0; i < 3; i++) {
		free(brightnesses[i]);
		free(contrasts[i]);
		free(gammas[i]);
		brightnesses[i] = NULL;
		contrasts[i] = NULL;
		gammas[i] = NULL;
	}
	free(crtcs);
	free(cards);
	free(framebuffers);
	crtcs = NULL;
	cards = NULL;
	framebuffers = NULL;
}
