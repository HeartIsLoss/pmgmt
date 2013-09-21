/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, RoboBusCreateDevice)
#endif


NTSTATUS
RoboBusCreateDevice(
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
    WDF_OBJECT_ATTRIBUTES   deviceAttributes;
    FDO_DEVICE_CONTEXT* deviceContext;
    WDFDEVICE device;
    NTSTATUS status;
	PNP_BUS_INFORMATION busInfo;

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, FDO_DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status))
	{
		WDF_OBJECT_ATTRIBUTES attributes;

		deviceContext = getFdoContext(device);

	    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = device;
		status = WdfWaitLockCreate( &attributes, &deviceContext->ChildLock );
	}

	if( NT_SUCCESS(status) )
	{
        //
        // Create a device interface so that applications can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_RoboBus,
            NULL // ReferenceString
            );
	}



    if (NT_SUCCESS(status)) {
        //
        // Initialize the I/O Package and any Queues
        //
        status = RoboBusQueueInitialize(device);
    }


    busInfo.BusTypeGuid = GUID_DEVCLASS_ROBOT;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;
    WdfDeviceSetBusInformationForChildren(device, &busInfo);

    return status;
}


