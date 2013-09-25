/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, RoboDeviceCreateDevice)
#endif


NTSTATUS
RoboDeviceCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    DEVICE_CONTEXT* pCtx;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFDEVICE device;
    NTSTATUS status;

    PAGED_CODE();

    scope
    {
        // pnpPowerCallbacks structure
        WDF_PNPPOWER_EVENT_CALLBACKS pnpPwrCallbacks;
        WDF_PNPPOWER_EVENT_CALLBACKS_INIT( &pnpPwrCallbacks );
        pnpPwrCallbacks.EvtDevicePrepareHardware = RoboDeviceEvtDevicePrepareHardware;
        pnpPwrCallbacks.EvtDeviceReleaseHardware = RoboDeviceEvtDeviceReleaseHardware;
        pnpPwrCallbacks.EvtDeviceD0Entry = RoboDeviceEvtDeviceD0Entry;
        pnpPwrCallbacks.EvtDeviceD0Exit = RoboDeviceEvtDeviceD0Exit;
        WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPwrCallbacks);
    }


    scope {
        // power policy callback used to arm/disarm hardware to handle wait-wake
        WDF_POWER_POLICY_EVENT_CALLBACKS powerPolicyCallbacks;
        WDF_POWER_POLICY_EVENT_CALLBACKS_INIT( &powerPolicyCallbacks );
        powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = RoboDeviceEvtDeviceArmWakeFromS0;
        powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = RoboDeviceEvtDeviceDisarmWakeFromS0;
        powerPolicyCallbacks.EvtDeviceWakeFromS0Triggered = RoboDeviceEvtDeviceWakeFromS0Triggered;
        powerPolicyCallbacks.EvtDeviceArmWakeFromSx = RoboDeviceEvtDeviceArmWakeFromSx;
        powerPolicyCallbacks.EvtDeviceDisarmWakeFromSx = RoboDeviceEvtDeviceDisarmWakeFromSx;
        powerPolicyCallbacks.EvtDeviceWakeFromSxTriggered = RoboDeviceEvtDeviceWakeFromSxTriggered;
        WdfDeviceInitSetPowerPolicyEventCallbacks( DeviceInit, &powerPolicyCallbacks );
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE( &attributes, DEVICE_CONTEXT );
    attributes.EvtCleanupCallback = RoboDeviceEvtDeviceContextCleanup;

    status = WdfDeviceCreate( &DeviceInit, &attributes, &device );
    if( !NT_SUCCESS(status) ) {
        DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "WdfDeviceCreate failed: 0x%x", status );
        return status;
    }

    // Initialize the context
    pCtx = DeviceGetContext( device );
    pCtx->PrivateDeviceData = 0;

    status = WdfDeviceCreateDeviceInterface( device, &GUID_DEVINTERFACE_RoboDevice, NULL );
    if( !NT_SUCCESS(status) ) {
        DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Error Create device interface: 0x%x", status );
        return status;
    }

    status = RoboDeviceQueueInitialize( device );


    scope {
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
        WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT( &idleSettings, IdleCannotWakeFromS0 );
        idleSettings.IdleTimeout = 60000;
        status = WdfDeviceAssignS0IdleSettings( device, &idleSettings );
    }


    scope {
        WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
        WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT( &wakeSettings );
        WdfDeviceAssignSxWakeSettings( device, &wakeSettings );
    }

    return status;
}




// Typically on IRP_MN_START_DEVICE
NTSTATUS
RoboDeviceEvtDevicePrepareHardware(
    WDFDEVICE      Device,
    WDFCMRESLIST   ResourcesRaw,
    WDFCMRESLIST   ResourcesTranslated
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;

    // pCtx

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesRaw);
    DbgPrint( "RoboDeviceEvtDevicePrepareHardware called" );
    PAGED_CODE();

    //
    // Get the number item that are currently in Resources collection and
    // iterate thru as many times to get more information about the each items
    //

    for (i=0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch(descriptor->Type)
        {

        case CmResourceTypePort:
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "I/O Port: (%x) Lenth: (%d)\n", 
                    descriptor->u.Port.Start.LowPart,
                    descriptor->u.Port.Length );
            break;

        case CmResourceTypeMemory:
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Memory: (%x) Lenth: (%d)\n", 
                    descriptor->u.Memory.Start.LowPart,
                    descriptor->u.Memory.Length );
            break;

        case CmResourceTypeInterrupt:
            DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
                "Interrupt level: 0x%0x, Vector: 0x%0x, Affinity: 0x%0Ix\n",
                descriptor->u.Interrupt.Level,
                descriptor->u.Interrupt.Vector,
                descriptor->u.Interrupt.Affinity );
            break;

        default:
            break;
        }

    }

    return status;

}
// IRP_MN_STOP_DEVICE, IRP_MN_REMOVE_DEVICE
NTSTATUS
RoboDeviceEvtDeviceReleaseHardware(
    IN  WDFDEVICE    Device,
    IN  WDFCMRESLIST ResourcesTranslated
    )
{
    //PFDO_DATA   fdoData;

    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    info0( "RoboDeviceEvtDeviceReleaseHardware called\n" );

    PAGED_CODE();

    // here - cleanup resources. Unmap memory and I/O ports

    return STATUS_SUCCESS;
}

// before device is removed in response to IRP_MN_REMOVE_DEVICE
VOID
RoboDeviceEvtDeviceContextCleanup(
    IN WDFOBJECT Device
    )
{
    //PFDO_DATA   fdoData;
    UNREFERENCED_PARAMETER(Device);

    info0( "RoboDeviceEvtDeviceContextCleanup called\n" );

    PAGED_CODE();

    return;
}
