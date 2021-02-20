/* See LICENSE file for copyright and license details. */
#include "common.h"


/**
 * The number of elements to allocates to a buffer for a DRM device pathname
 */
#define DRM_DEV_NAME_MAX_LEN\
  ((sizeof(DRM_DEV_NAME) + sizeof(DRM_DIR_NAME)) / sizeof(char) + 3 * sizeof(int))



/**
 * Figure out how many graphics cards there are on the system
 * 
 * @return  The number of graphics cards on the system
 */
size_t
drm_card_count(void)
{
	char buf[DRM_DEV_NAME_MAX_LEN];
	size_t count = 0;
	for (;; count++) {
		sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, (int)count);
		if (access(buf, F_OK) < 0)
			return count;
	}
}

/**
 * Acquire access to a graphics card
 * 
 * @param   index  The index of the graphics card
 * @param   card   Graphics card information to fill in
 * @return         Zero on success, -1 on error
 */
int
drm_card_open(size_t index, drm_card_t *restrict card)
{
	char buf[DRM_DEV_NAME_MAX_LEN];
	int old_errno;
	size_t i, n;

	card->fd = -1;
	card->res = NULL;
	card->connectors = NULL;
	card->encoders = NULL;
	card->connector_count = 0;

	sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, (int)index);
	card->fd = open(buf, O_RDWR);
	if (card->fd < 0)
		goto fail;

	card->res = drmModeGetResources(card->fd);
	if (!card->res)
		goto fail;

	card->crtc_count      = (size_t)(card->res->count_crtcs);
	card->connector_count = (size_t)(card->res->count_connectors);
	n = card->connector_count;

	card->connectors = calloc(n, sizeof(drmModeConnector*));
	if (!card->connectors)
		goto fail;
	card->encoders   = calloc(n, sizeof(drmModeEncoder*));
	if (!card->encoders)
		goto fail;

	for (i = 0; i < n; i++) {
		card->connectors[i] = drmModeGetConnector(card->fd, card->res->connectors[i]);
		if (!card->connectors[i])
			goto fail;

		if (card->connectors[i]->encoder_id) {
			card->encoders[i] = drmModeGetEncoder(card->fd, card->connectors[i]->encoder_id);
			if (!card->encoders[i])
				goto fail;
		}
	}

	return 0;
fail:
	old_errno = errno;
	drm_card_close(card);
	errno = old_errno;
	return -1;
}


/**
 * Release access to a graphics card
 * 
 * @param  card  The graphics card information
 */
void
drm_card_close(drm_card_t *restrict card)
{
	size_t i, n = card->connector_count;

	if (card->encoders) {
		for (i = 0; i < n; i++)
			if (card->encoders[i])
				drmModeFreeEncoder(card->encoders[i]);
		free(card->encoders);
		card->encoders = NULL;
	}

	if (card->connectors) {
		for (i = 0; i < n; i++)
			if (card->connectors[i])
				drmModeFreeConnector(card->connectors[i]);
		free(card->connectors);
		card->connectors = NULL;
	}

	if (card->res) {
		drmModeFreeResources(card->res);
		card->res = NULL;
	}
	if (card->fd >= 0) {
		close(card->fd);
		card->fd = -1;
	}
}


/**
 * Acquire access to a CRT controller
 * 
 * @param   index  The index of the CRT controller
 * @param   card   The graphics card information
 * @param   crtc   CRT controller information to fill in
 * @return         Zero on success, -1 on error
 */
int
drm_crtc_open(size_t index, drm_card_t *restrict card, drm_crtc_t *restrict crtc)
{
	drmModePropertyRes *restrict prop;
	drmModePropertyBlobRes *restrict blob;
	drmModeCrtc *restrict info;
	size_t i, j;
	int old_errno;
	unsigned char c;

	crtc->edid  = NULL;
	crtc->red   = NULL;
	crtc->green = NULL;
	crtc->blue  = NULL;

	crtc->id = card->res->crtcs[index];
	crtc->card = card;

	crtc->connector = NULL;
	crtc->encoder = NULL;
	for (i = 0; i < card->connector_count; i++) {
		if (card->encoders[i]) {
			if (card->encoders[i]->crtc_id == crtc->id) {
				crtc->connector = card->connectors[i];
				crtc->encoder = card->encoders[i];
			}
		}
	}

	crtc->connected = crtc->connector && crtc->connector->connection == DRM_MODE_CONNECTED;

	info = drmModeGetCrtc(card->fd, crtc->id);
	if (!info)
		return -1;
	crtc->gamma_stops = (size_t)info->gamma_size;
	drmModeFreeCrtc(info);

	/* `calloc` is for some reason required when reading the gamma ramps. */
	crtc->red = calloc(3 * crtc->gamma_stops, sizeof(uint16_t));
	if (!crtc->red)
		return -1;
	crtc->green = crtc->red   + crtc->gamma_stops;
	crtc->blue  = crtc->green + crtc->gamma_stops;

	if (!crtc->connector)
		return 0;
	for (i = 0; i < (size_t)crtc->connector->count_props; i++) {
		prop = drmModeGetProperty(card->fd, crtc->connector->props[i]);
		if (!prop)
			continue;

		if (strcmp(prop->name, "EDID"))
			goto free_prop;

		blob = drmModeGetPropertyBlob(card->fd, (uint32_t)crtc->connector->prop_values[i]);
		i = (size_t)crtc->connector->count_props;
		if (!blob || !blob->data)
			goto free_blob;

		crtc->edid = malloc((blob->length * 2 + 1) * sizeof(char));
		if (!crtc->edid) {
			old_errno = errno;
			drmModeFreePropertyBlob(blob);
			drmModeFreeProperty(prop);
			free(crtc->red);
			crtc->red = NULL;
			errno = old_errno;
			return -1;
		}
		for (j = 0; j < blob->length; j++) {
			c = ((unsigned char *)blob->data)[j];
			crtc->edid[j * 2 + 0] = "0123456789ABCDEF"[(c >> 4) & 15];
			crtc->edid[j * 2 + 1] = "0123456789ABCDEF"[(c >> 0) & 15];
		}
		crtc->edid[blob->length * 2] = '\0';

	free_blob:
		if (blob)
			drmModeFreePropertyBlob(blob);

	free_prop:
		drmModeFreeProperty(prop);
	}

	return 0;
}


/**
 * Release access to a CRT controller
 * 
 * @param  crtc  The CRT controller information to fill in
 */
void
drm_crtc_close(drm_crtc_t *restrict crtc)
{
	free(crtc->edid);
	free(crtc->red);
	crtc->edid = NULL;
	crtc->red = NULL;
}


/**
 * Read the gamma ramps for a CRT controller
 * 
 * @param   crtc  CRT controller information
 * @return        Zero on success, -1 on error 
 */
int
drm_get_gamma(drm_crtc_t *restrict crtc)
{
	return -!!drmModeCrtcGetGamma(crtc->card->fd, crtc->id, (uint32_t)crtc->gamma_stops, crtc->red, crtc->green, crtc->blue);
}


/**
 * Apply gamma ramps for a CRT controller
 * 
 * @param   crtc  CRT controller information
 * @return        Zero on success, -1 on error
 */
int
drm_set_gamma(drm_crtc_t *restrict crtc)
{
	return -!!drmModeCrtcSetGamma(crtc->card->fd, crtc->id, (uint32_t)crtc->gamma_stops, crtc->red, crtc->green, crtc->blue);
}
