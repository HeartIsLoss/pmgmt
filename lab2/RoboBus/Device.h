/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

typedef struct _PDO_DEVICE_CONTEXT
{
    ULONG PrivateDeviceData;  // just a placeholder

} PDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME( PDO_DEVICE_CONTEXT, GetPdoContext )



typedef struct _FDO_DEVICE_CONTEXT
{
	WDFWAITLOCK ChildLock;

} FDO_DEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME( FDO_DEVICE_CONTEXT, GetFdoContext )



//
// Function to initialize the device and its callbacks
//
NTSTATUS
RoboBusCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );

