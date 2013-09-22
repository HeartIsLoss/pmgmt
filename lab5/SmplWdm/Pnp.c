
#include <wdm.h>
#include "Driver.h"

extern PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS
SwdmDispatchPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    IO_STACK_LOCATION* pStk;
    DEVICE_EXTENSION* pCtx;
	ULONG MinorCode;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MJ_PNP Received : Begin\n" );

	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
	IoAcquireRemoveLock( &pCtx->RemoveLock, Irp);
	MinorCode = pStk->MinorFunction;

    DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Minor code: 0x%x\n", MinorCode );

	switch( MinorCode )
	{
	case IRP_MN_START_DEVICE:
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_START_DEVICE Received : Begin\r\n");
		Status = StartDevice( DeviceObject, Irp );
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_START_DEVICE Received : End\r\n");
		break;
	case IRP_MN_STOP_DEVICE:
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_STOP_DEVICE Received : Begin\r\n");
		Status = StopDevice( DeviceObject, Irp );
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_STOP_DEVICE Received : End\r\n");
		break;
	case IRP_MN_REMOVE_DEVICE:
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_REMOVE_DEVICE Received : Begin\r\n");
		Status = RemoveDevice( DeviceObject, Irp );
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_REMOVE_DEVICE Received : End\r\n");
		break;
	case IRP_MN_QUERY_CAPABILITIES :
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_QUERY_CAPABILITIES Received : Begin\r\n");
		Status = QueryCapability(
			DeviceObject,
			Irp
		);
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_QUERY_CAPABILITIES Received : End\r\n");
		break;
    case IRP_MN_QUERY_BUS_INFORMATION:
        DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_QUERY_BUS_INFORMATION Received : Begin\n");
        Status = QueryBus( DeviceObject, Irp );
        break;

	default:
		break;
	}
	IoReleaseRemoveLock( &pCtx->RemoveLock, Irp);
	
	if (NT_SUCCESS(Status))
    {
	  CompleteRequest(
		Irp, 
		STATUS_SUCCESS, 
		0);
    }

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MJ_PNP Received : End\r\n");

	return Status;
}

NTSTATUS
StartDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Start Device....\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS
StopDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Stop Device....\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS
RemoveDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	//PDEVICE_EXTENSION pCtx;
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Remove Device....\r\n" );
	return STATUS_SUCCESS;
}

NTSTATUS
QueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	NTSTATUS Status;
	KEVENT event;
	ULONG IdxPwrState;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Query Device Capability....\r\n");
	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass IRP down\r\n");
	
	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = DeviceObject->DeviceExtension;

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(
		Irp,
		(PIO_COMPLETION_ROUTINE)CompletionQueryCapability,
		(PVOID)&event,//pCtx,
		TRUE,
		TRUE,
		TRUE);

	Status = IoCallDriver(
				pCtx->NextDeviceObject,
				Irp);

	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		pStk = IoGetCurrentIrpStackLocation(Irp);
		pCtx = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
		pCtx->pdc = *pStk->Parameters.DeviceCapabilities.Capabilities;
		Global_PowerInfo_Ptr->SupportQueryCapability = TRUE;
		Global_PowerInfo_Ptr->DeviceD1 = pCtx->pdc.DeviceD1;
		Global_PowerInfo_Ptr->DeviceD2 = pCtx->pdc.DeviceD1;
		Global_PowerInfo_Ptr->WakeFromD0 = pCtx->pdc.WakeFromD0;
		Global_PowerInfo_Ptr->WakeFromD1 = pCtx->pdc.WakeFromD1;
		Global_PowerInfo_Ptr->WakeFromD2 = pCtx->pdc.WakeFromD2;
		Global_PowerInfo_Ptr->WakeFromD3 = pCtx->pdc.WakeFromD3;
		Global_PowerInfo_Ptr->DeviceWake = pCtx->pdc.DeviceWake;
		Global_PowerInfo_Ptr->SystemWake = pCtx->pdc.SystemWake;
		for (IdxPwrState = 0; 
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			Global_PowerInfo_Ptr->DeviceState[IdxPwrState] = 
				pCtx->pdc.DeviceState[IdxPwrState];
		}
	} else {
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Failed to handle IRP_MN_QUERY_CAPABILITIES");
	}
	return Irp->IoStatus.Status;
}

NTSTATUS
CompletionQueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	//It is Okay Version
	PIO_STACK_LOCATION pStk;
	//PDEVICE_EXTENSION pCtx;
	//NTSTATUS Status = STATUS_SUCCESS;
	//PIO_STATUS_BLOCK p_IO_Status;
	//ULONG IdxPwrState;
	PKEVENT pev;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In Completion Routine for IRP_MN_QUERY_CAPABILITIES....\r\n");
	pStk = IoGetCurrentIrpStackLocation(Irp);

	pev = (PKEVENT)Context;
	KeSetEvent(pev, 0, FALSE);
	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "return from IoCompletion Routine for IRP_MN_QUERY_CAPABILITIES\r\n");
	return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
QueryBus(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	NTSTATUS Status;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Query Bus Capability....\r\n");
	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass IRP down\r\n");
	
	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    IoSkipCurrentIrpStackLocation( Irp );
    Status = IoCallDriver( pCtx->NextDeviceObject, Irp );

    return Status;


}

