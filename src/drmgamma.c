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
  
  card->fd = -1;
  card->res = NULL;
  
  sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, (int)index);
  card->fd = open(buf, O_RDWR | O_CLOEXEC);
  if (card->fd == -1)
    goto fail;
  
  card->res = drmModeGetResources(card->fd);
  if (card->res == NULL)
    goto fail;
  
  card->crtc_count = (size_t)(card->res->count_crtcs);
  
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
  if (card->res != NULL)
    drmModeFreeResources(card->res), card->res = NULL;
  if (card->fd != -1)
    close(card->fd), card->fd = -1;
}
