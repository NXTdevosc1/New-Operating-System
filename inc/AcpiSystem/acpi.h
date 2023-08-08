#pragma once
#include "acpidef.h"




BOOLEAN ACPISUBSYSTEM AcpiGetVersion(UINT32* Version);

void ACPISUBSYSTEM AcpiShutdownSystem();

BOOLEAN ACPISUBSYSTEM AcpiGetTable(char* Signature, UINT32 Instance, void** Table);

BOOLEAN ACPISUBSYSTEM AcpiGetTableByIndex(UINT32 TableIndex, void** Table);