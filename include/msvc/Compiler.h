#ifndef COMPILER_H_
#define COMPILER_H_

/* unsupported keywords */
#define __interrupt
#define __near
#define __far
#define __attribute__(x)


#ifndef near
#define near        __near
#endif

#ifndef far
#define far         __far
#endif

/* autosar defines */
#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

#define INLINE __inline
#define LOCAL_INLINE
#define FUNC(rettype, memclass) memclass rettype

#endif /*COMPILER_H_*/
