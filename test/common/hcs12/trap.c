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

#include "Os.h"

#pragma CONST_SEG .vectors
#pragma CONST_SEG DEFAULT

#ifdef __HIWARE__
#define MCU_AT_ADDRESS(x) @(x)
#else
/* must be define by linker script */
#define MCU_AT_ADDRESS(x) __attribute((section (x) ))
#endif

void __near Mcu_IsrDefault(void)
{
  while (1) {
    ; /* NOP */
  }
}

typedef void(*__near Mcu_IsrFun)(void);

const Mcu_IsrFun Mcu_InterruptVectorTable[] MCU_AT_ADDRESS(".vectors") = {
  &Mcu_IsrDefault, /* 0x40  0xFF80   ivVsi */
  &Mcu_IsrDefault, /* 0x41  0xFF82   ivVReserved62 */
  &Mcu_IsrDefault, /* 0x42  0xFF84   ivVatdcompare */
  &Mcu_IsrDefault, /* 0x43  0xFF86   ivVhti */
  &Mcu_IsrDefault, /* 0x44  0xFF88   ivVapi */
  &Mcu_IsrDefault, /* 0x45  0xFF8A   ivVlvi */
  &Mcu_IsrDefault, /* 0x46  0xFF8C   ivVpwmesdn */
  &Mcu_IsrDefault, /* 0x47  0xFF8E   ivVportp */
  &Mcu_IsrDefault, /* 0x48  0xFF90   ivVReserved55 */
  &Mcu_IsrDefault, /* 0x49  0xFF92   ivVReserved54 */
  &Mcu_IsrDefault, /* 0x4A  0xFF94   ivVReserved53 */
  &Mcu_IsrDefault, /* 0x4B  0xFF96   ivVReserved52 */
  &Mcu_IsrDefault, /* 0x4C  0xFF98   ivVReserved51 */
  &Mcu_IsrDefault, /* 0x4D  0xFF9A   ivVReserved50 */
  &Mcu_IsrDefault, /* 0x4E  0xFF9C   ivVReserved49 */
  &Mcu_IsrDefault, /* 0x4F  0xFF9E   ivVReserved48 */
  &Mcu_IsrDefault, /* 0x50  0xFFA0   ivVReserved47 */
  &Mcu_IsrDefault, /* 0x51  0xFFA2   ivVReserved46 */
  &Mcu_IsrDefault, /* 0x52  0xFFA4   ivVReserved45 */
  &Mcu_IsrDefault, /* 0x53  0xFFA6   ivVReserved44 */
  &Mcu_IsrDefault, /* 0x54  0xFFA8   ivVReserved43 */
  &Mcu_IsrDefault, /* 0x55  0xFFAA   ivVReserved42 */
  &Mcu_IsrDefault, /* 0x56  0xFFAC   ivVReserved41 */
  &Mcu_IsrDefault, /* 0x57  0xFFAE   ivVReserved40 */
  &Mcu_IsrDefault, /* 0x58  0xFFB0   ivVcantx */
  &Mcu_IsrDefault, /* 0x59  0xFFB2   ivVcanrx */
  &Mcu_IsrDefault, /* 0x5A  0xFFB4   ivVcanerr */
  &Mcu_IsrDefault, /* 0x5B  0xFFB6   ivVcanwkup */
  &Mcu_IsrDefault, /* 0x5C  0xFFB8   ivVflash */
  &Mcu_IsrDefault, /* 0x5D  0xFFBA   ivVflashfd */
  &Mcu_IsrDefault, /* 0x5E  0xFFBC   ivVReserved33 */
  &Mcu_IsrDefault, /* 0x5F  0xFFBE   ivVReserved32 */
  &Mcu_IsrDefault, /* 0x60  0xFFC0   ivVReserved31 */
  &Mcu_IsrDefault, /* 0x61  0xFFC2   ivVReserved30 */
  &Mcu_IsrDefault, /* 0x62  0xFFC4   ivVsci2 */
  &Mcu_IsrDefault, /* 0x63  0xFFC6   ivVcpmuplllck */
  &Mcu_IsrDefault, /* 0x64  0xFFC8   ivVcpmuocsns */
  &Mcu_IsrDefault, /* 0x65  0xFFCA   ivVReserved26 */
  &Mcu_IsrDefault, /* 0x66  0xFFCC   ivVReserved25 */
  &Mcu_IsrDefault, /* 0x67  0xFFCE   ivVportj */
  &Mcu_IsrDefault, /* 0x68  0xFFD0   ivVReserved23 */
  &Mcu_IsrDefault, /* 0x69  0xFFD2   ivVatd */
  &Mcu_IsrDefault, /* 0x6A  0xFFD4   ivVsci1 */
  &Mcu_IsrDefault, /* 0x6B  0xFFD6   ivVsci0 */
  &Mcu_IsrDefault, /* 0x6C  0xFFD8   ivVspi */
  &Mcu_IsrDefault, /* 0x6D  0xFFDA   ivVtimpaie */
  &Mcu_IsrDefault, /* 0x6E  0xFFDC   ivVtimpaaovf */
  &Mcu_IsrDefault, /* 0x6F  0xFFDE   ivVtimovf */
  &Mcu_IsrDefault, /* 0x70  0xFFE0   ivVtimch7 */
  &Mcu_IsrDefault, /* 0x71  0xFFE2   ivVtimch6 */
  &Mcu_IsrDefault, /* 0x72  0xFFE4   ivVtimch5 */
  &Mcu_IsrDefault, /* 0x73  0xFFE6   ivVtimch4 */
  &Mcu_IsrDefault, /* 0x74  0xFFE8   ivVtimch3 */
  &Mcu_IsrDefault, /* 0x75  0xFFEA   ivVtimch2 */
  &Mcu_IsrDefault, /* 0x76  0xFFEC   ivVtimch1 */
  &Mcu_IsrDefault, /* 0x77  0xFFEE   ivVtimch0 */
  &Os_Arch_Isr   , /* 0x78  0xFFF0   ivVrti */
  &Mcu_IsrDefault, /* 0x79  0xFFF2   ivVirq */
  &Mcu_IsrDefault, /* 0x7A  0xFFF4   ivVxirq */
  &Os_Arch_Swi   , /* 0x7B  0xFFF6   ivVswi */
  &Mcu_IsrDefault  /* 0x7C  0xFFF8   ivVtrap */
};

void TERMIO_PutChar(char c)
{

}