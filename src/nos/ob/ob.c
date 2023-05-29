#include <nos/ob/obutil.h>

/*
Allocates an object in the object table
This mechanism is used to make it easier for the Ob Subsystem to find an object by its addresss
*/

/*
The Object manager uses the quarter of the top of the address space
*/

/*
Each ob map contains 63 objects and 512 bytes of space
*/
/*
It is headed by a full key
*/

typedef struct {
    void* Objects[512];
} OBPAGE;


HANDLE ObCreateObject(
    void* Object
) {
    
}