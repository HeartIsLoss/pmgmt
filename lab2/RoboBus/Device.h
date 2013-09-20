/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _FDO_DEVICE_CONTEXT
{
	WDFWAITLOCK ChildLock;
} FDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DEVICE_CONTEXT, getFdoContext);

typedef struct _PDO_DEVICE_CONTEXT
{
	ULONG SerialNo;
} PDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(PDO_DEVICE_CONTEXT, getPdoContext);


//
// Function to initialize the device and its callbacks
//
NTSTATUS
RoboBusCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

