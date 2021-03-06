
#ifndef STD_TYPES_H_
#define STD_TYPES_H_

#include "Platform_Types.h"
#include "Compiler.h"

#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
    typedef uint8 StatusType;
    #define E_OK    (StatusType)0x00u
#endif

typedef StatusType  Std_ReturnType;

#define E_NOT_OK    (Std_ReturnType)0x01u

#define STD_HIGH    0x01
#define STD_LOW     0x00

#define STD_ACTIVE  0x01
#define STD_IDLE    0x00

#define STD_ON      0x01
#define STD_OFF     0x00

#define STD_TRUE    0x01
#define STD_FALSE   0x00


typedef struct {
    uint16 VendorId;
    uint16 ModuleId;
    uint16 SwMajorVersion;
    uint16 SwMinorVersion;
    uint16 SwPatchVersion;
} Std_VersionInfoType;

#endif /*STD_TYPES_H_*/
