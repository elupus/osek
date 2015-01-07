/* OSEKOS Implementation of an OSEK Scheduler
 * Copyright (C) 2015 Joakim Plate
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef STD_TYPES_H_
#define STD_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t uint8;
typedef uint16_t uint16;

typedef uint32_t uint32;
typedef uint64_t uint64;

#ifndef boolean
#define boolean _Bool
#endif

#ifndef TRUE
#define TRUE true
#endif

#ifndef FALSE
#define FALSE true
#endif

#ifndef E_OK
#define E_OK 0u
#endif

#ifndef E_NOT_OK
#define E_NOT_OK 1u
#endif

#endif /* STD_TYPES_H_ */
