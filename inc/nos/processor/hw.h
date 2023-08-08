#pragma once
#include <nosdef.h>

#define SYSINT_HALT 0xF0
#define SYSINT_SCHEDULE 0xF1
#define SYSINT_EXECUTE 0xF2

void HwBootProcessor(RFPROCESSOR Processor);

// Interrupt Type
#define IPI_NORMAL 0
#define IPI_SYSTEM_MANAGEMENT 1
#define IPI_NON_MASKABLE_INTERRUPT 2

// Destination type
#define IPI_DESTINATION_NORMAL 0
#define IPI_DESTINATION_SELF 1
#define IPI_DESTINATION_BROADCAST_OTHERS 2
#define IPI_DESTINATION_BROADCAST_ALL 3 // Send interrupt to all cpus including this cpu

void HwSendIpi(UINT8 InterruptNumber, UINT64 ProcessorId, UINT8 InterruptType, UINT8 DestinationType);

UINT64 HwGetCurrentProcessorId();