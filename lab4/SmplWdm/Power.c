/*************************************************************************
/*
/*      Copyright 2004/02/08
/*      Author : Meng-Jer Tsai(Taiwan R.O.C)
/*      Email : mjtsai87@ms31.hinet.net
/*      Declaration and Agreement Statements:
/*      [1]This Program is free for redistribution with
/*         or without any modify of the source code containing
/*         in this source file on conditions that the Declaration
/*         and Agreement Statements is included.
/*      [2]If any damanage done to your computer system due to the
/*         execution of the source code in this file, no responsibility
/*         will be taken by the Author.
/*      [3]Any commercial binary executable product developed based on
/*         this source code must have the Copyright and all items in
/*         this Declaration and Agreement Statements in its User License
/*         Agreement, or will be regarded as inappropriate occupy of
/*         software creativity.
/*
/*************************************************************************/

/*************************************************************************
/*
/*       This file is implemented for the common and default handle for
/*       Power Management-Related tasks, such as Query/Set Power.
/*
/*************************************************************************/

#include <wdm.h>
#include "Driver.h"

extern PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS
  PsdoDispatchPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	ULONG IRP_MN_Code;
	POWER_STATE PowerState;
	NTSTATUS Status;

	DbgPrint("IRP_MJ_POWER Received : Begin\r\n");

	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	IRP_MN_Code = p_IO_STK->MinorFunction;
	p_DVCEXT = DeviceObject->DeviceExtension;
	Status = IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, Irp);
	switch (IRP_MN_Code)
	{
	case IRP_MN_SET_POWER : 
		DbgPrint("System or device power is going to be change\r\n");
		if (!NT_SUCCESS(Status))
		{
			DbgPrint("Failed in Acquire Remove Lock\r\n");
			PoStartNextPowerIrp(Irp);
			CompleteRequest(Irp, Status, 0);
			return Status;
		}
		//Determine the Power Setting is for system or device?
		//The IRP is for system power state change
		if (p_IO_STK->Parameters.Power.Type == SystemPowerState)
		{
			DbgPrint("System power state change\r\n");
			DbgPrint(
				"System power state = %d\r\n",
				p_IO_STK->Parameters.Power.State.SystemState);
			//Maintain system power state if new system power state
			//is not equal to prior system power state
			if (p_DVCEXT->SysPwrState != 
				p_IO_STK->Parameters.Power.State.SystemState)
			{
				p_DVCEXT->SysPwrState = 
					p_IO_STK->Parameters.Power.State.SystemState;
			}
			//Ready to pass down IRP for system power state change
			DbgPrint("Ready to pass down IRP for system power state change");
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionSetSystemPower,
				(PVOID) p_DVCEXT,
				TRUE, TRUE, TRUE);
			IoMarkIrpPending(Irp);
			PoCallDriver(
				p_DVCEXT->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		//The IRP is for device power state change
		if (p_IO_STK->Parameters.Power.Type == DevicePowerState)
		{
			DbgPrint("Device power state change\r\n");
			DbgPrint(
				"Device power state = %d\r\n",
				p_IO_STK->Parameters.Power.State.DeviceState);
			//Determine if this power setting is for power up or down the device?
			if ((p_DVCEXT->DevPwrState >= p_IO_STK->Parameters.Power.State.DeviceState)
				||(PowerDeviceD0 == p_IO_STK->Parameters.Power.State.DeviceState))
			{
				//Device power up
				DbgPrint("Device power up\r\n");
				DbgPrint("Ready to pass down IRP for device power state change");
				IoMarkIrpPending(Irp);
				IoCopyCurrentIrpStackLocationToNext(Irp);
				IoSetCompletionRoutine(
					Irp,
					(PIO_COMPLETION_ROUTINE) CompletionDevicePowerUp,
					(PVOID) p_DVCEXT,
					TRUE, TRUE, TRUE);
				PoCallDriver(
					p_DVCEXT->NextDeviceObject,
					Irp);
				return STATUS_PENDING;
			}
			if ((p_DVCEXT->DevPwrState >= p_IO_STK->Parameters.Power.State.DeviceState)
				||(PowerDeviceD3 == p_IO_STK->Parameters.Power.State.DeviceState)
				||(p_IO_STK->Parameters.Power.ShutdownType == PowerSystemSleeping3)//Stand by
				||(p_IO_STK->Parameters.Power.ShutdownType == PowerActionHibernate)
				||(p_IO_STK->Parameters.Power.ShutdownType == PowerActionShutdown)
				||(p_IO_STK->Parameters.Power.ShutdownType == PowerActionShutdownReset)
				||(p_IO_STK->Parameters.Power.ShutdownType == PowerActionShutdownOff))
			{
				//Device power down
				DbgPrint("Device power down\r\n");
				DbgPrint("Ready to pass down IRP for device power state change");
				//Maintain current device power state in driver
				p_DVCEXT->DevPwrState = 
					p_IO_STK->Parameters.Power.State.DeviceState;
				PowerState.DeviceState = 
					p_IO_STK->Parameters.Power.State.DeviceState;
				PoSetPowerState(
					DeviceObject,
					DevicePowerState,
					PowerState);
				PoStartNextPowerIrp(Irp);
				IoSkipCurrentIrpStackLocation(Irp);
				PoCallDriver(
					p_DVCEXT->NextDeviceObject,
					Irp);
				IoReleaseRemoveLock(
					&p_DVCEXT->RemoveLock,
					Irp);
				return STATUS_PENDING;
			}
		}
		break;
	case IRP_MN_QUERY_POWER :
		DbgPrint("Power Manager query power state\r\n");
		if (!NT_SUCCESS(Status))
		{
			DbgPrint("Failed in Acquire Remove Lock\r\n");
			PoStartNextPowerIrp(Irp);
			CompleteRequest(Irp, Status, 0);
			return Status;
		}
		//Determine the power query is for system or device?
		//Query for the incoming system power state change
		if (p_IO_STK->Parameters.Power.Type == SystemPowerState)
		{
			DbgPrint("System power state query\r\n");
			DbgPrint(
				"System power state = %d\r\n",
				p_IO_STK->Parameters.Power.State.SystemState);
			//Ready to pass down IRP for system power state query
			DbgPrint("Ready to pass down IRP for system power state query");
			IoMarkIrpPending(Irp);
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionQuerySystemPower,
				(PVOID) p_DVCEXT,
				TRUE, TRUE, TRUE);
			PoCallDriver(
				p_DVCEXT->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		//Query for the incoming device power state change
		if (p_IO_STK->Parameters.Power.Type == DevicePowerState)
		{
			DbgPrint("Device power state query");
			DbgPrint(
				"Device power state = %d\r\n",
				p_IO_STK->Parameters.Power.State.DeviceState);
			//Ready to pass down IRP for device power state query
			DbgPrint("Ready to pass down IRP for device power state query");
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionQueryDevicePower,
				(PVOID) p_DVCEXT,
				TRUE, TRUE, TRUE);
			PoCallDriver(
				p_DVCEXT->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		break;
	case IRP_MN_WAIT_WAKE : 
		DbgPrint("IRP_MN_WAIT_WAKE Received\r\n");
		break;
	case IRP_MN_POWER_SEQUENCE : 
		DbgPrint("IRP_MN_POWER_SEQUENCE Received\r\n");
		break;
	default :
		break;
	}
	//Do nothing to Power event
	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
	CompleteRequest(Irp, STATUS_SUCCESS, 0);

	DbgPrint("IRP_MJ_POWER Received : End\r\n");

	return STATUS_SUCCESS;
}

NTSTATUS
  CompletionQuerySystemPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	POWER_STATE PowerState;
	ULONG DevicePowerIndex;
	NTSTATUS Status;

	DbgPrint("In completion routine for IRP_MN_QUERY_POWER for system power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrint("The lower-layered driver failed IRP_MN_QUERY_POWER handling for system power state\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}

	//Send IRP_MN_QUERY_POWER for Device Power State
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	DevicePowerIndex = 
		p_IO_STK->Parameters.Power.State.SystemState;
	PowerState.DeviceState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	p_DVCEXT->PowerIrp = Irp;
	Status = PoRequestPowerIrp(
				DeviceObject,
				IRP_MN_QUERY_POWER,
				PowerState,
				(PREQUEST_POWER_COMPLETE)CallBackForRequestPower,
				Context,
				NULL);
	ASSERT(Status == STATUS_PENDING);
	if (Status == STATUS_PENDING)
	{
		DbgPrint("IRP_MN_QUERY_POWER for device power has been send successfully");
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else
	{
		DbgPrint("Send IRP_MN_QUERY_POWER for Device Power failed");
		IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
		CompleteRequest(Irp, Status, 0);
		return Status;
	}
}

VOID
  CallBackForRequestPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
	PDEVICE_EXTENSION p_DVCEXT;
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrint("In callback routine for PoRequestPowerIrp of device power query\r\n");

	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	PoStartNextPowerIrp(p_DVCEXT->PowerIrp);
	IoReleaseRemoveLock(
		&p_DVCEXT->RemoveLock,
		p_DVCEXT->PowerIrp);
	CompleteRequest(
		p_DVCEXT->PowerIrp,
		IoStatus->Status,
		IoStatus->Information);
}

NTSTATUS
  CompletionQueryDevicePower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	PDEVICE_EXTENSION p_DVCEXT;
    UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrint("In completion routine for IRP_MN_QUERY_POWER for device power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrint("The lower-layered driver failed IRP_MN_QUERY_POWER handling for Device Power State\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}

	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	PoStartNextPowerIrp(Irp);
	IoReleaseRemoveLock(
		&p_DVCEXT->RemoveLock,
		Irp);
	CompleteRequest(
		Irp,
		STATUS_SUCCESS,
		0);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
  CompletionSetSystemPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	POWER_STATE PowerState;
	ULONG DevicePowerIndex;
	NTSTATUS Status;

	DbgPrint("In completion routine for IRP_MN_SET_POWER for system power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrint("The lower-layered driver failed IRP_MN_SET_POWER handling for System Power State\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}
	//Send IRP_MN_SET_POWER for Device Power State
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	DevicePowerIndex = 
		p_IO_STK->Parameters.Power.State.SystemState;
	PowerState.DeviceState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	p_DVCEXT->PowerIrp = Irp;
	Status = PoRequestPowerIrp(
				DeviceObject,
				IRP_MN_SET_POWER,
				PowerState,
				(PREQUEST_POWER_COMPLETE)CallBackForSetPower,
				Context,
				NULL);
	ASSERT(Status == STATUS_PENDING);
	if (Status == STATUS_PENDING)
	{
		DbgPrint("IRP_MN_SET_POWER for device power has been send successfully");
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else
	{
		DbgPrint("Send IRP_MN_SET_POWER for Device Power failed");
		IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
		CompleteRequest(Irp, Status, 0);
		return Status;
	}
}

VOID
  CallBackForSetPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	ULONG DevicePowerIndex;
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(MinorFunction);

	DbgPrint("In callback routine for PoRequestPowerIrp of device power setting\r\n");

	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	p_IO_STK = IoGetCurrentIrpStackLocation(p_DVCEXT->PowerIrp);

	//Maintain current system and device power state in drivers
	DevicePowerIndex = 
		p_IO_STK->Parameters.Power.State.SystemState;
	p_DVCEXT->SysPwrState = 
		p_IO_STK->Parameters.Power.State.SystemState;
	p_DVCEXT->DevPwrState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	//Ready to handle next power IRP
	PoStartNextPowerIrp(p_DVCEXT->PowerIrp);
	IoReleaseRemoveLock(
		&p_DVCEXT->RemoveLock,
		p_DVCEXT->PowerIrp);
	CompleteRequest(
		p_DVCEXT->PowerIrp,
		IoStatus->Status,
		IoStatus->Information);
}

NTSTATUS
  CompletionDevicePowerUp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	POWER_STATE PowerState;

	DbgPrint("In completion routine for IRP_MN_SET_POWER for device power");

	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = (PDEVICE_EXTENSION)Context;
	p_DVCEXT->DevPwrState = p_IO_STK->Parameters.Power.State.DeviceState;
	PowerState.DeviceState = p_IO_STK->Parameters.Power.State.DeviceState;
	ASSERT(p_DVCEXT->DevPwrState == PowerDeviceD0);
	PoSetPowerState(
		DeviceObject,
		DevicePowerState,
		PowerState);
	PoStartNextPowerIrp(Irp);
	IoReleaseRemoveLock(
		&p_DVCEXT->RemoveLock,
		p_DVCEXT->PowerIrp);
	return STATUS_SUCCESS;
}
