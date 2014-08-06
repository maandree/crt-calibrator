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
#ifndef CRT_CALIBRATOR_DRMGAMMA_H
#define CRT_CALIBRATOR_DRMGAMMA_H


#include <xf86drm.h>
#include <xf86drmMode.h>


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
  drmModeRes* restrict res;
  
  /**
   * The number of CRTC:s available on the graphics card
   */
  size_t crtc_count;
  
  /**
   * The available connectors
   */
  drmModeConnector** restrict connectors;
  
  /**
   * The available encoders
   */
  drmModeEncoder** restrict encoders;
  
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
  drm_card_t* restrict card;
  
  /**
   * The CRT controller's connector
   */
  drmModeConnector* restrict connector;
  
  /**
   * The CRT controller's encoder
   */
  drmModeEncoder* restrict encoder;
  
  /**
   * Whether the connector is connected
   */
  int connected;
  
  /**
   * The CRT's EDID, hexadecimally encoded
   */
  char* edid;
  
} drm_crtc_t;


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
int drm_card_open(size_t index, drm_card_t* restrict card);

/**
 * Release access to a graphics card
 * 
 * @param  card  The graphics card information
 */
void drm_card_close(drm_card_t* restrict card);

/**
 * Acquire access to a CRT controller
 * 
 * @param   index  The index of the CRT controller
 * @param   card   The graphics card information
 * @param   crtc   CRT controller information to fill in
 * @return         Zero on success, -1 on error
 */
int drm_crtc_open(size_t index, drm_card_t* restrict card, drm_crtc_t* restrict crtc);

/**
 * Release access to a CRT controller
 * 
 * @param  crtc  The CRT controller information to fill in
 */
void drm_crtc_close(drm_crtc_t* restrict crtc);


#endif

