#ifndef COMPILER_H_
#define COMPILER_H_

#ifndef NULL_PTR
#define NULL_PTR ((void *)0)
#endif

/* unsupported keywords */
#define __inline
#define __attribute(x)

/* autosar defines */
/* #define INLINE - clashes with pragma */
#define LOCAL_INLINE
#define FUNC(rettype, memclass) memclass rettype

#endif /*COMPILER_H_*/
