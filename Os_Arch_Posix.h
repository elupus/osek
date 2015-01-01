/*
 * Os_Arch_Posix.h
 *
 *  Created on: 31 dec 2014
 *      Author: joakim
 */

#ifndef OS_ARCH_POSIX_H_
#define OS_ARCH_POSIX_H_

void       Os_Arch_Init(void);

void       Os_Arch_DisableAllInterrupts(void);
void       Os_Arch_EnableAllInterrupts(void);

void       Os_Arch_StoreState(Os_TaskType task);
void       Os_Arch_RestoreState(Os_TaskType task);
void       Os_Arch_PrepareState(Os_TaskType task);


#endif /* OS_ARCH_POSIX_H_ */
