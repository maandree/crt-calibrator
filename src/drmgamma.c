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
#include "drmgamma.h"

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>



/**
 * Close on exec flag for `open`
 */
#ifndef O_CLOEXEC
# define O_CLOEXEC  02000000
#endif


/**
 * The number of elements to allocates to a buffer for a DRM device pathname
 */
#define DRM_DEV_NAME_MAX_LEN  \
  ((sizeof(DRM_DEV_NAME) + sizeof(DRM_DIR_NAME)) / sizeof(char) + 3 * sizeof(int))



/**
 * Figure out how many graphics cards there are on the system
 * 
 * @return  The number of graphics cards on the system
 */
size_t drm_card_count(void)
{
  char buf[DRM_DEV_NAME_MAX_LEN];
  size_t count = 0;
  
  for (;; count++)
    {
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
int drm_card_open(size_t index, drm_card_t* restrict card)
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
  card->fd = open(buf, O_RDWR | O_CLOEXEC);
  if (card->fd == -1)
    goto fail;
  
  card->res = drmModeGetResources(card->fd);
  if (card->res == NULL)
    goto fail;
  
  card->crtc_count      = (size_t)(card->res->count_crtcs);
  card->connector_count = (size_t)(card->res->count_connectors);
  n = card->connector_count;
  
  card->connectors = calloc(n, sizeof(drmModeConnector*));
  if (card->connectors == NULL)
    goto fail;
  card->encoders   = calloc(n, sizeof(drmModeEncoder*));
  if (card->encoders == NULL)
    goto fail;
  
  for (i = 0; i < n; i++)
    {
      card->connectors[i] = drmModeGetConnector(card->fd, card->res->connectors[i]);
      if (card->connectors[i] == NULL)
	goto fail;
      
      if (card->connectors[i]->encoder_id != 0)
	{
	  card->encoders[i] = drmModeGetEncoder(card->fd, card->connectors[i]->encoder_id);
	  if (card->encoders[i] == NULL)
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
void drm_card_close(drm_card_t* restrict card)
{
  size_t i, n = card->connector_count;
  
  if (card->encoders != NULL)
    for (i = 0; i < n; i++)
      if (card->encoders[i] != NULL)
	drmModeFreeEncoder(card->encoders[i]);
  free(card->encoders), card->encoders = NULL;
  
  if (card->connectors != NULL)
    for (i = 0; i < n; i++)
      if (card->connectors[i] != NULL)
	drmModeFreeConnector(card->connectors[i]);
  free(card->connectors), card->connectors = NULL;
  
  if (card->res != NULL)
    drmModeFreeResources(card->res), card->res = NULL;
  if (card->fd != -1)
    close(card->fd), card->fd = -1;
}


/**
 * Acquire access to a CRT controller
 * 
 * @param   index  The index of the CRT controller
 * @param   card   The graphics card information
 * @param   crtc   CRT controller information to fill in
 * @return         Zero on success, -1 on error
 */
int drm_crtc_open(size_t index, drm_card_t* restrict card, drm_crtc_t* restrict crtc)
{
  drmModePropertyRes* restrict prop;
  drmModePropertyBlobRes* restrict blob;
  size_t i;
  
  crtc->edid = NULL;
  
  crtc->id = card->res->crtcs[index];
  crtc->card = card;
  
  for (i = 0; i < card->connector_count; i++)
    if (card->encoders[i] != NULL)
      if (card->encoders[i]->crtc_id == crtc->id)
	{
	  crtc->connector = card->connectors[i];
	  crtc->encoder = card->encoders[i];
	}
  
  crtc->connected = crtc->connector->connection == DRM_MODE_CONNECTED;
  
  for (i = 0; i < crtc->connector->count_props; i++)
    {
      size_t j;
      
      prop = drmModeGetProperty(card->fd, crtc->connector->props[i]);
      if (prop == NULL)
        continue;
      
      if (strcmp(prop->name, "EDID"))
	goto free_prop;
      
      
      blob = drmModeGetPropertyBlob(card->fd, (uint32_t)(crtc->connector->prop_values[i]));
      i = crtc->connector->count_props;
      if ((blob == NULL) || (blob->data == NULL))
	goto free_blob;
      
      crtc->edid = malloc((blob->length * 2 + 1) * sizeof(char));
      if (crtc->edid == NULL)
	{
	  drmModeFreePropertyBlob(blob);
	  drmModeFreeProperty(prop);
	  return -1;
	}
      for (j = 0; j < blob->length; j++)
	{
	  unsigned char c = ((unsigned char*)(blob->data))[j];
	  crtc->edid[j * 2 + 0] = "0123456789ABCDEF"[(c >> 4) & 15];
	  crtc->edid[j * 2 + 1] = "0123456789ABCDEF"[(c >> 0) & 15];
	}
      crtc->edid[blob->length * 2] = '\0';
      
    free_blob:
      if (blob != NULL)
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
void drm_crtc_close(drm_crtc_t* restrict crtc)
{
  free(crtc->edid), crtc->edid = NULL;
}

