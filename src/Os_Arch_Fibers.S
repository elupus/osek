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

 	.global Os_Arch_Trap
 	.global Os_Arch_Isr


 	.text

Os_Arch_Trap:
#if defined(__x86_64)
 	call Os_Arch_Isr
 	pop %rax
 	pop %rcx
 	pop %rdx
 	pop %r8
 	pop %r9
 	pop %r10
 	pop %r11
 	add $8, %rsp /* ret */
 	jmp *-8(%rsp)
#endif

