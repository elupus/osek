/* OSEKOS Implementation of an OSEK Scheduler
 * Copyright (C) 2026 Joakim Plate
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

/**
 * @file
 *
 * ThreadX configuration for the metric apps on QEMU's virt machine, with
 * the same tick as the OSEK metric apps and the features they do not use
 * left out. Read by ThreadX and the apps through
 * TX_INCLUDE_USER_DEFINE_FILE.
 */

#ifndef TX_USER_H_
#define TX_USER_H_

/* same tick as the OSEK metric apps: 100 Hz from the CLINT machine timer */
#define TX_TIMER_TICKS_PER_SECOND               (100UL)

/* expire timers in the tick interrupt, as OSEK alarms and FreeRTOS
 * delays, instead of in a timer thread */
#define TX_TIMER_PROCESS_IN_ISR

/* leave out everything the metric apps do not use, as the OSEK builds
 * have extended error checking off: the tx_* services map straight to
 * the _tx_* implementations without parameter checks */
#define TX_DISABLE_ERROR_CHECKING
#define TX_DISABLE_PREEMPTION_THRESHOLD
#define TX_DISABLE_NOTIFY_CALLBACKS
#define TX_DISABLE_STACK_FILLING
#define TX_NO_FILEX_POINTER

#endif /* TX_USER_H_ */
