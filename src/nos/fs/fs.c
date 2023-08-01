#include <nos/nos.h>
#include <nos/fs/fs.h>

HANDLE KeOpenFile(
    PEPROCESS Process,
    UINT16* Path,
    UINT OpenType,
    UINT64 DesiredAccess
) {

}

BOOLEAN KeSetFileOffset(HANDLE File, UINT64 Offset) {

}

BOOLEAN KeReadFile(HANDLE File, UINT64 NumBytes, void* Buffer) {

}

BOOLEAN KeWriteFile(HANDLE File, UINT64 NumBytes, void* Buffer) {

}

BOOLEAN KeEditFileInfo(HANDLE File, UINT InformationType, void* Information) {

}
