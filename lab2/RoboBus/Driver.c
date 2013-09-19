/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, RoboBusEvtDeviceAdd)
#pragma alloc_text (PAGE, RoboBusEvtDriverContextCleanup)
#endif


NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Entry point to the driver. Initialize the callbacks.

Parameters Description:

    DriverObject - pointer to driver object

	RegistryPath - respresents the driver specific path in the Registry.

Return Value:

    NT status code

--*/
{
    WDF_DRIVER_CONFIG config;
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;

	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "RoboBus Driver Entry\n" );
	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "Built %s %s\n", __DATE__, __TIME__ );

    //
    // Register a cleanup callback.
    WDF_OBJECT_ATTRIBUTES_INIT( &attributes );
    attributes.EvtCleanupCallback = RoboBusEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT( &config, RoboBusEvtDeviceAdd );

    status = WdfDriverCreate( DriverObject, RegistryPath, &attributes, &config, WDF_NO_HANDLE );

    if (!NT_SUCCESS(status)) {
		DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfDriverCreate failed: 0x%x", status );
        return status;
    }

	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "Exit Driver Entry %x", status );

    return status;
}

NTSTATUS
RoboBusEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the RoboBus functional device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Driver);
    PAGED_CODE();

	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "RoboBusEvtDeviceAdd: 0x%p\n", Driver );

    status = RoboBusCreateDevice(DeviceInit);


    return status;
}

VOID
RoboBusEvtDriverContextCleanup(
    _In_ WDFOBJECT DriverObject
    )
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    DriverObject - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE ();

}
