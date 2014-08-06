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


/**
 * Figure out how many framebuffers there are on the system
 * 
 * @return  The number of framebuffers on the system
 */
size_t fb_count(void);


#endif

