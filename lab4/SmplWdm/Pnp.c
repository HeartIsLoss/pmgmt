
#include <wdm.h>
#include "Driver.h"

extern PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS
  PsdoDispatchPnP(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	ULONG IRP_MN_Code;
	NTSTATUS Status = STATUS_SUCCESS;

	DbgPrint("IRP_MJ_PNP Received : Begin\r\n");

	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;
	IoAcquireRemoveLock(
		&p_DVCEXT->RemoveLock, 
		Irp);
	IRP_MN_Code = p_IO_STK->MinorFunction ;

    DbgPrint( "Minor code: 0x%x\n", IRP_MN_Code );

	switch(IRP_MN_Code)
	{
	case IRP_MN_START_DEVICE :
		DbgPrint("IRP_MN_START_DEVICE Received : Begin\r\n");
		Status = StartDevice(
			DeviceObject,
			Irp
		);
		DbgPrint("IRP_MN_START_DEVICE Received : End\r\n");
		break;
	case IRP_MN_STOP_DEVICE :
		DbgPrint("IRP_MN_STOP_DEVICE Received : Begin\r\n");
		Status = StopDevice(
			DeviceObject,
			Irp
		);
		DbgPrint("IRP_MN_STOP_DEVICE Received : End\r\n");
		break;
	case IRP_MN_REMOVE_DEVICE :
		DbgPrint("IRP_MN_REMOVE_DEVICE Received : Begin\r\n");
		Status = RemoveDevice(
			DeviceObject,
			Irp
		);
		DbgPrint("IRP_MN_REMOVE_DEVICE Received : End\r\n");
		break;
	case IRP_MN_QUERY_CAPABILITIES :
		DbgPrint("IRP_MN_QUERY_CAPABILITIES Received : Begin\r\n");
		Status = QueryCapability(
			DeviceObject,
			Irp
		);
		DbgPrint("IRP_MN_QUERY_CAPABILITIES Received : End\r\n");
		break;
    case IRP_MN_QUERY_BUS_INFORMATION:
        DbgPrint("IRP_MN_QUERY_BUS_INFORMATION Received : Begin\n");
        Status = QueryBus( DeviceObject, Irp );
        break;

	default:
		break;
	}
	IoReleaseRemoveLock(
		&p_DVCEXT->RemoveLock, 
		Irp);
	
	if (NT_SUCCESS(Status))
	  CompleteRequest(
		Irp, 
		STATUS_SUCCESS, 
		0);

	DbgPrint("IRP_MJ_PNP Received : End\r\n");

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
	DbgPrint("Start Device....\r\n");
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
	DbgPrint("Stop Device....\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS
  RemoveDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	//PDEVICE_EXTENSION p_DVCEXT;
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

	DbgPrint("Remove Device....\r\n");
#if 0
	p_DVCEXT = DeviceObject->DeviceExtension;
	IoDetachDevice(
		p_DVCEXT->DeviceObject);
	IoDeleteDevice(
		p_DVCEXT->NextDeviceObject);
#endif
	return STATUS_SUCCESS;
}

NTSTATUS
  QueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
#if 1
	//It is OKAY Version
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	NTSTATUS Status;
	KEVENT event;
	ULONG IdxPwrState;

	DbgPrint("Query Device Capability....\r\n");
	DbgPrint("Ready to pass IRP down\r\n");
	
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(
		Irp,
		(PIO_COMPLETION_ROUTINE)CompletionQueryCapability,
		(PVOID)&event,//p_DVCEXT,
		TRUE,
		TRUE,
		TRUE);

	Status = IoCallDriver(
				p_DVCEXT->NextDeviceObject,
				Irp);

	KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
		p_DVCEXT = DeviceObject->DeviceExtension;
		p_DVCEXT->pdc = *p_IO_STK->
			Parameters.DeviceCapabilities.Capabilities;
#if 1
		Global_PowerInfo_Ptr->SupportQueryCapability = TRUE;
		Global_PowerInfo_Ptr->DeviceD1 = p_DVCEXT->pdc.DeviceD1;
		Global_PowerInfo_Ptr->DeviceD2 = p_DVCEXT->pdc.DeviceD1;
		Global_PowerInfo_Ptr->WakeFromD0 = p_DVCEXT->pdc.WakeFromD0;
		Global_PowerInfo_Ptr->WakeFromD1 = p_DVCEXT->pdc.WakeFromD1;
		Global_PowerInfo_Ptr->WakeFromD2 = p_DVCEXT->pdc.WakeFromD2;
		Global_PowerInfo_Ptr->WakeFromD3 = p_DVCEXT->pdc.WakeFromD3;
		Global_PowerInfo_Ptr->DeviceWake = p_DVCEXT->pdc.SystemWake;
		Global_PowerInfo_Ptr->SystemWake = p_DVCEXT->pdc.DeviceWake;
		for (IdxPwrState = 0; 
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			Global_PowerInfo_Ptr->DeviceState[IdxPwrState] = 
				p_DVCEXT->pdc.DeviceState[IdxPwrState];
		}
#else
		//Display Related Device Power State
		DbgPrint("WakeFromD0 : %d\r\n", p_DVCEXT->pdc.WakeFromD0);
		DbgPrint("WakeFromD1 : %d\r\n", p_DVCEXT->pdc.WakeFromD1);
		DbgPrint("WakeFromD2 : %d\r\n", p_DVCEXT->pdc.WakeFromD2);
		DbgPrint("WakeFromD3 : %d\r\n", p_DVCEXT->pdc.WakeFromD3);
		DbgPrint("SystemWake : %d\r\n", p_DVCEXT->pdc.SystemWake);
		DbgPrint("DeviceWake : %d\r\n", p_DVCEXT->pdc.DeviceWake);
		for (IdxPwrState = 0; 
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			DbgPrint("DeviceState[%d] : %d\r\n", 
				IdxPwrState, 
				p_DVCEXT->pdc.DeviceState[IdxPwrState]);
		}
#endif
	} else {
		DbgPrint("Failed to handle IRP_MN_QUERY_CAPABILITIES");
	}
	return Irp->IoStatus.Status;
#else
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	NTSTATUS Status;

	DbgPrint("Query Device Capability....\r\n");
	DbgPrint("Ready to pass IRP down\r\n");
	
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(
		Irp,
		(PIO_COMPLETION_ROUTINE)CompletionQueryCapability,
		p_DVCEXT,
		TRUE,
		TRUE,
		TRUE);

	return IoCallDriver(
				p_DVCEXT->NextDeviceObject,
				Irp);
/*
	if (Status == STATUS_PENDING)
	{
		DbgPrint("IRP_MN_QUERY_CAPABILITIES requires more preocessing...\r\n");
		return STATUS_PENDING;
	}
	else
	{
		DbgPrint("IoCallDriver return status :%d\r\n", Status);
		return Status;
	}
*/
#endif
}

NTSTATUS
  CompletionQueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
#if 1
	//It is Okay Version
	PIO_STACK_LOCATION p_IO_STK;
	//PDEVICE_EXTENSION p_DVCEXT;
	//NTSTATUS Status = STATUS_SUCCESS;
	//PIO_STATUS_BLOCK p_IO_Status;
	//ULONG IdxPwrState;
	PKEVENT pev;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

	DbgPrint("In Completion Routine for IRP_MN_QUERY_CAPABILITIES....\r\n");
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);

	pev = (PKEVENT)Context;
	KeSetEvent(pev, 0, FALSE);
	DbgPrint("return from IoCompletion Routine for IRP_MN_QUERY_CAPABILITIES\r\n");
	return STATUS_MORE_PROCESSING_REQUIRED;
#else
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STATUS_BLOCK p_IO_Status;
	ULONG IdxPwrState;

	DbgPrint("In Completion Routine for IRP_MN_QUERY_CAPABILITIES....\r\n");
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);

	//Check if IRP is completed by lower-layered driver 
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		DbgPrint("IRP_MN_QUERY_CAPABILITIES completed by lower-layered driver\r\n");
		p_DVCEXT = DeviceObject->DeviceExtension;
		p_DVCEXT->pdc = *p_IO_STK->
			Parameters.DeviceCapabilities.Capabilities;
		//Display Related Device Power State
		DbgPrint("WakeFromD0 : %d\r\n", p_DVCEXT->pdc.WakeFromD0);
		DbgPrint("WakeFromD1 : %d\r\n", p_DVCEXT->pdc.WakeFromD1);
		DbgPrint("WakeFromD2 : %d\r\n", p_DVCEXT->pdc.WakeFromD2);
		DbgPrint("WakeFromD3 : %d\r\n", p_DVCEXT->pdc.WakeFromD3);
		DbgPrint("SystemWake : %d\r\n", p_DVCEXT->pdc.SystemWake);
		DbgPrint("DeviceWake : %d\r\n", p_DVCEXT->pdc.DeviceWake);
		for (IdxPwrState = 0; 
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			DbgPrint("DeviceState[%d] : %d\r\n", 
				IdxPwrState, 
				p_DVCEXT->pdc.DeviceState[IdxPwrState]);
		}
		if (Irp->PendingReturned)
		{
			IoMarkIrpPending(Irp);
			Status =  STATUS_PENDING;
		} else {
			Status = STATUS_SUCCESS;
		}
	} 
	else if (NT_ERROR(Irp->IoStatus.Status))
	{
		DbgPrint("error occur while lower-layered driver handle IRP_MN_QUERY_CAPABILITIES\r\n");
		Status = Irp->IoStatus.Status;
	}
	else {
		//Check if IRP is canceled by lower-layered driver
		if (Irp->Cancel) 
		{
			DbgPrint("IRP_MN_QUERY_CAPABILITIES canceled by lower-layered driver\r\n");
			Status = Irp->IoStatus.Status;
		}
	}
	//CompleteRequest(Irp, Status, 0);
	DbgPrint("return from IoCompletion Routine for IRP_MN_QUERY_CAPABILITIES\r\n");
	return Status;
#endif
}

// ------------------------------

NTSTATUS
QueryBus(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	NTSTATUS Status;

	DbgPrint("Query Bus Capability....\r\n");
	DbgPrint("Ready to pass IRP down\r\n");
	
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    
    IoSkipCurrentIrpStackLocation( Irp );
    Status = IoCallDriver( p_DVCEXT->NextDeviceObject, Irp );

    return Status;


}

NTSTATUS
CompletionQueryBus(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	//It is Okay Version
	PIO_STACK_LOCATION p_IO_STK;
	//PDEVICE_EXTENSION p_DVCEXT;
	//NTSTATUS Status = STATUS_SUCCESS;
	//PIO_STATUS_BLOCK p_IO_Status;
	//ULONG IdxPwrState;
	PKEVENT pev;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

	DbgPrint("In Completion Routine for IRP_MN_QUERY_CAPABILITIES....\r\n");
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);

	pev = (PKEVENT)Context;
	KeSetEvent(pev, 0, FALSE);
	DbgPrint("return from IoCompletion Routine for IRP_MN_QUERY_CAPABILITIES\r\n");
	return STATUS_MORE_PROCESSING_REQUIRED;
}


