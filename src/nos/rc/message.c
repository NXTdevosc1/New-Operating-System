#include <nos/nos.h>
#include <rc/rc.h>
#include <ob/obutil.h>



NSTATUS KRNLAPI RcSend(HANDLE Port, UINT Flags, void* Data, UINT64 SizeOfData, BOOLEAN* Completed) {
    
}
BOOLEAN KRNLAPI RcReceive(HANDLE Port, PRCHEADER RcHeader);