#pragma once
#include <nosdef.h>


#ifndef ACPISUBSYSTEM
#define ACPISUBSYSTEM __declspec(dllimport) __fastcall
#endif

#define ACPI_OEM_ID_SIZE                6
#define ACPI_OEM_TABLE_ID_SIZE          8

#define ACPI_NAMESEG_SIZE               4           /* Fixed by ACPI spec */

typedef INT64 ACPISTATUS;

typedef struct acpi_table_header
{
    char                    Signature[ACPI_NAMESEG_SIZE];       /* ASCII table signature */
    UINT32                  Length;                             /* Length of table in bytes, including this header */
    UINT8                   Revision;                           /* ACPI Specification minor version number */
    UINT8                   Checksum;                           /* To make sum of entire table == 0 */
    char                    OemId[ACPI_OEM_ID_SIZE];            /* ASCII OEM identification */
    char                    OemTableId[ACPI_OEM_TABLE_ID_SIZE]; /* ASCII OEM table identification */
    UINT32                  OemRevision;                        /* OEM revision number */
    char                    AslCompilerId[ACPI_NAMESEG_SIZE];   /* ASCII ASL compiler vendor ID */
    UINT32                  AslCompilerRevision;                /* ASL compiler version */

} ACPI_TABLE_HEADER;