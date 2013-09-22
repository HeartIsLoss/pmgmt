#include <wdm.h>
#include "Driver.h"

extern PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS
  PsdoDispatchPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	ULONG IRP_MN_Code;
	POWER_STATE PowerState;
	NTSTATUS Status;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MJ_POWER Received : Begin\r\n");

	pStk = IoGetCurrentIrpStackLocation(Irp);
	IRP_MN_Code = pStk->MinorFunction;
	pCtx = DeviceObject->DeviceExtension;
	Status = IoAcquireRemoveLock(&pCtx->RemoveLock, Irp);
	switch (IRP_MN_Code)
	{
	case IRP_MN_SET_POWER : 
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "System or device power is going to be change\r\n");
		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Failed in Acquire Remove Lock\r\n");
			PoStartNextPowerIrp(Irp);
			CompleteRequest(Irp, Status, 0);
			return Status;
		}
		//Determine the Power Setting is for system or device?
		//The IRP is for system power state change
		if (pStk->Parameters.Power.Type == SystemPowerState)
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "System power state change\r\n");
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
				"System power state = %d\r\n",
				pStk->Parameters.Power.State.SystemState);
			//Maintain system power state if new system power state
			//is not equal to prior system power state
			if (pCtx->SysPwrState != 
				pStk->Parameters.Power.State.SystemState)
			{
				pCtx->SysPwrState = 
					pStk->Parameters.Power.State.SystemState;
			}
			//Ready to pass down IRP for system power state change
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass down IRP for system power state change");
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionSetSystemPower,
				(PVOID) pCtx,
				TRUE, TRUE, TRUE);
			IoMarkIrpPending(Irp);
			PoCallDriver(
				pCtx->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		//The IRP is for device power state change
		if (pStk->Parameters.Power.Type == DevicePowerState)
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device power state change\r\n");
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
				"Device power state = %d\r\n",
				pStk->Parameters.Power.State.DeviceState);
			//Determine if this power setting is for power up or down the device?
			if ((pCtx->DevPwrState >= pStk->Parameters.Power.State.DeviceState)
				||(PowerDeviceD0 == pStk->Parameters.Power.State.DeviceState))
			{
				//Device power up
				DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device power up\r\n");
				DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass down IRP for device power state change");
				IoMarkIrpPending(Irp);
				IoCopyCurrentIrpStackLocationToNext(Irp);
				IoSetCompletionRoutine(
					Irp,
					(PIO_COMPLETION_ROUTINE) CompletionDevicePowerUp,
					(PVOID) pCtx,
					TRUE, TRUE, TRUE);
				PoCallDriver(
					pCtx->NextDeviceObject,
					Irp);
				return STATUS_PENDING;
			}
			if ((pCtx->DevPwrState >= pStk->Parameters.Power.State.DeviceState)
				||(PowerDeviceD3 == pStk->Parameters.Power.State.DeviceState)
				||(pStk->Parameters.Power.ShutdownType == PowerSystemSleeping3)//Stand by
				||(pStk->Parameters.Power.ShutdownType == PowerActionHibernate)
				||(pStk->Parameters.Power.ShutdownType == PowerActionShutdown)
				||(pStk->Parameters.Power.ShutdownType == PowerActionShutdownReset)
				||(pStk->Parameters.Power.ShutdownType == PowerActionShutdownOff))
			{
				//Device power down
				DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device power down\r\n");
				DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass down IRP for device power state change");
				//Maintain current device power state in driver
				pCtx->DevPwrState = 
					pStk->Parameters.Power.State.DeviceState;
				PowerState.DeviceState = 
					pStk->Parameters.Power.State.DeviceState;
				PoSetPowerState(
					DeviceObject,
					DevicePowerState,
					PowerState);
				PoStartNextPowerIrp(Irp);
				IoSkipCurrentIrpStackLocation(Irp);
				PoCallDriver(
					pCtx->NextDeviceObject,
					Irp);
				IoReleaseRemoveLock(
					&pCtx->RemoveLock,
					Irp);
				return STATUS_PENDING;
			}
		}
		break;

	case IRP_MN_QUERY_POWER :
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Power Manager query power state\r\n");
		if (!NT_SUCCESS(Status))
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Failed in Acquire Remove Lock\r\n");
			PoStartNextPowerIrp(Irp);
			CompleteRequest(Irp, Status, 0);
			return Status;
		}
		//Determine the power query is for system or device?
		//Query for the incoming system power state change
		if (pStk->Parameters.Power.Type == SystemPowerState)
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "System power state query\r\n");
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
				"System power state = %d\r\n",
				pStk->Parameters.Power.State.SystemState);
			//Ready to pass down IRP for system power state query
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass down IRP for system power state query");
			IoMarkIrpPending(Irp);
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionQuerySystemPower,
				(PVOID) pCtx,
				TRUE, TRUE, TRUE);
			PoCallDriver(
				pCtx->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		//Query for the incoming device power state change
		if (pStk->Parameters.Power.Type == DevicePowerState)
		{
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Device power state query");
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, 
				"Device power state = %d\r\n",
				pStk->Parameters.Power.State.DeviceState);
			//Ready to pass down IRP for device power state query
			DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Ready to pass down IRP for device power state query");
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine(
				Irp,
				(PIO_COMPLETION_ROUTINE) CompletionQueryDevicePower,
				(PVOID) pCtx,
				TRUE, TRUE, TRUE);
			PoCallDriver(
				pCtx->NextDeviceObject,
				Irp);
			return STATUS_PENDING;
		}
		break;
	case IRP_MN_WAIT_WAKE : 
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_WAIT_WAKE Received\r\n");
		break;
	case IRP_MN_POWER_SEQUENCE : 
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_POWER_SEQUENCE Received\r\n");
		break;
	default :
		break;
	}
	//Do nothing to Power event
	IoReleaseRemoveLock(&pCtx->RemoveLock, Irp);
	CompleteRequest(Irp, STATUS_SUCCESS, 0);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MJ_POWER Received : End\r\n");

	return STATUS_SUCCESS;
}

NTSTATUS
  CompletionQuerySystemPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    )
{
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	POWER_STATE PowerState;
	ULONG DevicePowerIndex;
	NTSTATUS Status;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In completion routine for IRP_MN_QUERY_POWER for system power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "The lower-layered driver failed IRP_MN_QUERY_POWER handling for system power state\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}

	//Send IRP_MN_QUERY_POWER for Device Power State
	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (PDEVICE_EXTENSION)Context;
	DevicePowerIndex = 
		pStk->Parameters.Power.State.SystemState;
	PowerState.DeviceState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	pCtx->PowerIrp = Irp;
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
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_QUERY_POWER for device power has been send successfully");
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else
	{
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Send IRP_MN_QUERY_POWER for Device Power failed");
		IoReleaseRemoveLock(&pCtx->RemoveLock, Irp);
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
	PDEVICE_EXTENSION pCtx;
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In callback routine for PoRequestPowerIrp of device power query\r\n");

	pCtx = (PDEVICE_EXTENSION)Context;
	PoStartNextPowerIrp(pCtx->PowerIrp);
	IoReleaseRemoveLock(
		&pCtx->RemoveLock,
		pCtx->PowerIrp);
	CompleteRequest(
		pCtx->PowerIrp,
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
	PDEVICE_EXTENSION pCtx;
    UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In completion routine for IRP_MN_QUERY_POWER for device power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "The lower-layered driver failed IRP_MN_QUERY_POWER handling for Device Power State\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}

	pCtx = (PDEVICE_EXTENSION)Context;
	PoStartNextPowerIrp(Irp);
	IoReleaseRemoveLock(
		&pCtx->RemoveLock,
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
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	POWER_STATE PowerState;
	ULONG DevicePowerIndex;
	NTSTATUS Status;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In completion routine for IRP_MN_SET_POWER for system power");

	if (!NT_SUCCESS(Irp->IoStatus.Status))
	{
		//The lower-layered driver failed in handling
		//IRP_MN_QUERY_POWER for System Power State
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "The lower-layered driver failed IRP_MN_SET_POWER handling for System Power State\r\n");
		CompleteRequest(Irp, Irp->IoStatus.Status, 0);
		return Irp->IoStatus.Status;
	}
	//Send IRP_MN_SET_POWER for Device Power State
	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (PDEVICE_EXTENSION)Context;
	DevicePowerIndex = 
		pStk->Parameters.Power.State.SystemState;
	PowerState.DeviceState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	pCtx->PowerIrp = Irp;
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
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IRP_MN_SET_POWER for device power has been send successfully");
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
	else
	{
		DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Send IRP_MN_SET_POWER for Device Power failed");
		IoReleaseRemoveLock(&pCtx->RemoveLock, Irp);
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
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	ULONG DevicePowerIndex;
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(MinorFunction);

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In callback routine for PoRequestPowerIrp of device power setting\r\n");

	pCtx = (PDEVICE_EXTENSION)Context;
	pStk = IoGetCurrentIrpStackLocation(pCtx->PowerIrp);

	//Maintain current system and device power state in drivers
	DevicePowerIndex = 
		pStk->Parameters.Power.State.SystemState;
	pCtx->SysPwrState = 
		pStk->Parameters.Power.State.SystemState;
	pCtx->DevPwrState = 
		Global_PowerInfo_Ptr->DeviceState[DevicePowerIndex];
	//Ready to handle next power IRP
	PoStartNextPowerIrp(pCtx->PowerIrp);
	IoReleaseRemoveLock(
		&pCtx->RemoveLock,
		pCtx->PowerIrp);
	CompleteRequest(
		pCtx->PowerIrp,
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
	PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	POWER_STATE PowerState;

	DbgPrintEx( DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In completion routine for IRP_MN_SET_POWER for device power");

	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (PDEVICE_EXTENSION)Context;
	pCtx->DevPwrState = pStk->Parameters.Power.State.DeviceState;
	PowerState.DeviceState = pStk->Parameters.Power.State.DeviceState;
	ASSERT(pCtx->DevPwrState == PowerDeviceD0);
	PoSetPowerState(
		DeviceObject,
		DevicePowerState,
		PowerState);
	PoStartNextPowerIrp(Irp);
	IoReleaseRemoveLock(
		&pCtx->RemoveLock,
		pCtx->PowerIrp);
	return STATUS_SUCCESS;
}
