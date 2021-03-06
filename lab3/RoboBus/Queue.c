/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "Public.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, RoboBusQueueInitialize)
#endif


NTSTATUS RoboBusPlugInDevice( WDFDEVICE Device, wchar_t* HardwareIds, ULONG SerialNo );
NTSTATUS RoboBusUnplugDevice( WDFDEVICE device, ULONG SerialNo );
NTSTATUS RoboBusEjectDevice( WDFDEVICE device, ULONG SerialNo );


NTSTATUS RoboBusCreatePdo( WDFDEVICE device, wchar_t* HardwareIds, ULONG SerialNo );



NTSTATUS
RoboBusQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;

    PAGED_CODE();
    
    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE( &queueConfig, WdfIoQueueDispatchParallel );
    queueConfig.EvtIoDeviceControl = RoboBusEvtIoDeviceControl;
    queueConfig.EvtIoStop = RoboBusEvtIoStop;

    status = WdfIoQueueCreate( Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue );

    if( !NT_SUCCESS(status) ) {
		DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfIoQueueCreate failed: %x", status );
    }

    return status;
}

VOID
RoboBusEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
	WDFDEVICE hdevice;
	BUSENUM_PLUGIN_HARDWARE* pplugin;
	BUSENUM_UNPLUG_HARDWARE* punplug;
	BUSENUM_EJECT_HARDWARE* peject;
	size_t length = 0;
	NTSTATUS status;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(OutputBufferLength);

	hdevice = WdfIoQueueGetDevice( Queue );
	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "RoboBusEvtIoDeviceControl 0x%p\n", hdevice );

	switch( IoControlCode )
	{
	case IOCTL_BUSENUM_PLUGIN_HARDWARE:
		status = WdfRequestRetrieveInputBuffer( Request, sizeof(BUSENUM_PLUGIN_HARDWARE) + sizeof(UNICODE_NULL)*2, (void**)&pplugin, &length );

		if( !NT_SUCCESS(status) ) {
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfRequestRetrieveInputBuffer failed 0x%x\n", status );
			break;
		}

		if( pplugin->Size == sizeof(BUSENUM_PLUGIN_HARDWARE)  )
		{
			length = ( InputBufferLength - sizeof(BUSENUM_PLUGIN_HARDWARE) ) / sizeof(wchar_t);

			if( ( pplugin->HardwareIDs[ length - 1 ] != UNICODE_NULL ) ||
				( pplugin->HardwareIDs[ length - 2 ] != UNICODE_NULL ) )
			{
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			status = RoboBusPlugInDevice( hdevice, pplugin->HardwareIDs, pplugin->SerialNo );
		}
		else
		{
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "Incorect size signature %d for plugin request", pplugin->Size );
			status = STATUS_INVALID_PARAMETER;
		}
		break;
	case IOCTL_BUSENUM_UNPLUG_HARDWARE:

		status = WdfRequestRetrieveInputBuffer( Request, sizeof(BUSENUM_UNPLUG_HARDWARE), (void**)&punplug, &length );
		if( !NT_SUCCESS(status) ) {
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfRequestRetrieveInputBuffer failed %x\n", status );
			break;
		}
		if( punplug->Size == InputBufferLength )
		{
			status = RoboBusUnplugDevice( hdevice, punplug->SerialNo );
		}
		else
		{
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "Incorect size signature %d for plugin request", punplug->Size );
			status = STATUS_INVALID_PARAMETER;
		}
		break;

    case IOCTL_BUSENUM_EJECT_HARDWARE:

        status = WdfRequestRetrieveInputBuffer( Request, sizeof (BUSENUM_EJECT_HARDWARE), (void**)&peject, &length );
		if( !NT_SUCCESS(status) ) {
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfRequestRetrieveInputBuffer failed %x\n", status );
			break;
		}
        if( peject->Size == InputBufferLength )
        {
            status= RoboBusEjectDevice( hdevice, peject->SerialNo );
        }
		else
		{
			DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "Incorect size signature %d for plugin request", peject->Size );
			status = STATUS_INVALID_PARAMETER;
		}
        break;

    default:
		status = STATUS_INVALID_PARAMETER;
        break;

	}

	WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}



NTSTATUS RoboBusPlugInDevice( WDFDEVICE device, wchar_t* HardwareIds, ULONG SerialNo )
{
	FDO_DEVICE_CONTEXT* fdoContext;
	WDFDEVICE hChild;
	BOOLEAN unique = TRUE;
	NTSTATUS status;

	PAGED_CODE();

	fdoContext = getFdoContext(device);
	hChild = NULL;

	WdfWaitLockAcquire( fdoContext->ChildLock, NULL );
	WdfFdoLockStaticChildListForIteration( device );

	while( (hChild = WdfFdoRetrieveNextStaticChild(device, hChild, WdfRetrieveAddedChildren)) != NULL )
	{
		PDO_DEVICE_CONTEXT* pdoContext;

		pdoContext = getPdoContext( hChild );
		if( SerialNo == pdoContext->SerialNo ) {
			unique = FALSE;
			status = STATUS_INVALID_PARAMETER;
			break;
		}
	}

	if( unique ) {
		status = RoboBusCreatePdo( device, HardwareIds, SerialNo );
	}

	WdfFdoUnlockStaticChildListFromIteration( device );
	WdfWaitLockRelease( fdoContext->ChildLock );

	UNREFERENCED_PARAMETER(device);
	UNREFERENCED_PARAMETER(HardwareIds);
	UNREFERENCED_PARAMETER(SerialNo);
	return STATUS_SUCCESS;
}


NTSTATUS RoboBusUnplugDevice( WDFDEVICE device, ULONG SerialNo )
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN fUnplugAll;
	WDFDEVICE hchild = NULL;
	BOOLEAN fFound = FALSE;

	PAGED_CODE();

	fUnplugAll = ( SerialNo == 0 ) ? TRUE : FALSE;

	WdfFdoLockStaticChildListForIteration( device );

	while( ( hchild = WdfFdoRetrieveNextStaticChild(device, hchild, WdfRetrieveAddedChildren) ) != NULL )
	{
		if( fUnplugAll ) {

            status = WdfPdoMarkMissing( hchild );
            if(!NT_SUCCESS(status)) {
				DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfPdoMarkMissing failed 0x%x\n", status );
				break;
			}

			fFound = TRUE;
        }

        else
		{
			PDO_DEVICE_CONTEXT* pdoContext = getPdoContext( hchild );
			if( SerialNo == pdoContext->SerialNo ) {

				status = WdfPdoMarkMissing( hchild );
				if(!NT_SUCCESS(status)) {
					DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "WdfPdoMarkMissing failed 0x%x\n", status );
					break;
				}

				fFound = TRUE;
				break;
			}
		}
    }

    WdfFdoUnlockStaticChildListFromIteration( device );

	if( NT_SUCCESS(status) ) {
	    status = fFound ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER;
	}

	return status;
}



NTSTATUS RoboBusEjectDevice( WDFDEVICE device, ULONG SerialNo )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN fEjectAll;
	WDFDEVICE hchild = NULL;

	PAGED_CODE();

	fEjectAll = ( SerialNo == 0 ) ? TRUE : FALSE;

	WdfFdoLockStaticChildListForIteration( device );

	while( ( hchild = WdfFdoRetrieveNextStaticChild(device, hchild, WdfRetrieveAddedChildren) ) != NULL )
	{
        PDO_DEVICE_CONTEXT* pdoContext = getPdoContext( hchild );

		if( fEjectAll || SerialNo == pdoContext->SerialNo ) {
            WdfPdoRequestEject( hchild );
            if( !fEjectAll ) {
                break;
            }
        }

    }

    WdfFdoUnlockStaticChildListFromIteration( device );


	return status;
}


VOID
RoboBusEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
	DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_INFO_LEVEL, "RoboBusEvtIoStop: Queue %p, Request %p, ActionFlags %d", Queue, Request, ActionFlags );

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}

