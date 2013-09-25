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
typedef struct _DEVICE_CONTEXT
{
    ULONG PrivateDeviceData;  // just a placeholder

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
RoboDeviceCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    );


EVT_WDF_DEVICE_PREPARE_HARDWARE RoboDeviceEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE RoboDeviceEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY RoboDeviceEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT RoboDeviceEvtDeviceD0Exit;


EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 RoboDeviceEvtDeviceArmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 RoboDeviceEvtDeviceDisarmWakeFromS0;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED RoboDeviceEvtDeviceWakeFromS0Triggered;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX RoboDeviceEvtDeviceArmWakeFromSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX RoboDeviceEvtDeviceDisarmWakeFromSx;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED RoboDeviceEvtDeviceWakeFromSxTriggered;

EVT_WDF_OBJECT_CONTEXT_CLEANUP RoboDeviceEvtDeviceContextCleanup;


#define scope
#define info0(fmt)           DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, fmt )
#define info1(fmt,x)         DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, fmt, x )
#define info2(fmt,x,y)       DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, fmt, x, y )
#define info3(fmt,x,y,z)     DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, fmt, x, y, z )
#define warn0(fmt)           DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_WARN_LEVEL, fmt )
#define warn1(fmt,x)         DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_WARN_LEVEL, fmt, x )
#define warn2(fmt,x,y)       DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_WARN_LEVEL, fmt, x, y )
#define warn3(fmt,x,y,z)     DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_WARN_LEVEL, fmt, x, y, z )
#define err0(fmt)            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt )
#define err1(fmt,x)          DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, x )
#define err2(fmt,x,y)        DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, x, y )
#define err3(fmt,x,y,z)      DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, x, y, z )

PCHAR DbgDevicePowerString( IN WDF_POWER_DEVICE_STATE Type );

