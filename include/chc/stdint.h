#ifndef STDINT_H_
#define STDINT_H_

#include <stdtypes.h>

typedef Byte  uint8_t;
typedef Word  uint16_t;
typedef LWord uint32_t;

typedef sByte  int8_t;
typedef sWord  int16_t;
typedef sLWord int32_t;

typedef uint8_t  uint_least8_t;
typedef uint16_t uint_least16_t;
typedef uint32_t uint_least32_t;

typedef int8_t  int_least8_t;
typedef int16_t int_least16_t;
typedef int32_t int_least32_t;

typedef int32_t  intptr_t;
typedef uint32_t uintptr_t;

#define INT16_MAX (32767)
#define INT16_MIN (-32767-1)
#define INT32_MAX (2147483647L)
#define INT32_MIN (-2147483647L-1)
#define INT64_MAX (9223372036854775807LL)
#define INT64_MIN (-9223372036854775807LL-1)

#define UINT8_MAX  (255U)
#define UINT16_MAX (65535U)
#define UINT32_MAX (4294967295UL)
#define UINT64_MAX (18446744073709551615ULL)

#endif