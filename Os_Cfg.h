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

#ifndef OS_CFG_H_
#define OS_CFG_H_

#ifdef OS_CFG_ARCH_POSIX
#include "Os_Arch_Posix.h"
#endif

#ifdef OS_CFG_ARCH_FIBERS
#include "Os_Arch_Fibers.h"
#endif

#define OS_TASK_COUNT 3
#define OS_PRIO_COUNT 3
#define OS_RES_COUNT  4

#define OS_TICK_US    1000000U

#define OS_PRETASKHOOK_ENABLE  0
#define OS_POSTTASKHOOK_ENABLE 0
#define OS_ERRORHOOK_ENABLE    1

#endif /* OS_CFG_H_ */
