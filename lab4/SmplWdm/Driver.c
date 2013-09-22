#include <wdm.h>

#include "Driver.h"


UNICODE_STRING Global_sz_Drv_RegInfo;
UNICODE_STRING Global_sz_DeviceName;
PDEVICE_POWER_INFORMATION Global_PowerInfo_Ptr;

NTSTATUS DriverEntry( 
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath 
	)
{
	RtlInitUnicodeString( &Global_sz_Drv_RegInfo, RegistryPath->Buffer);
	
	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = PsdoDispatchCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = PsdoDispatchClose;
	DriverObject->MajorFunction[IRP_MJ_READ] =  PsdoDispatchRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] =  PsdoDispatchWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PsdoDispatchDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_POWER] = PsdoDispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = PsdoDispatchPnP;

	return STATUS_SUCCESS;
}


NTSTATUS AddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject 
    )
{
	ULONG DeviceExtensionSize;
	PDEVICE_EXTENSION p_DVCEXT;
	PDEVICE_OBJECT ptr_PDO;
	NTSTATUS status;
	ULONG IdxPwrState;

	RtlInitUnicodeString(
		&Global_sz_DeviceName,
		L"\\DosDevices\\PSDOBUFDVC");
	//Get DEVICE_EXTENSION required memory space
	DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
	status = IoCreateDevice(
		DriverObject,
		DeviceExtensionSize,
		&Global_sz_DeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, 
		FALSE,
		&ptr_PDO
    );

	if (NT_SUCCESS(status)) {
		ptr_PDO->Flags &= ~DO_DEVICE_INITIALIZING;
		ptr_PDO->Flags |= DO_BUFFERED_IO;
		p_DVCEXT = ptr_PDO->DeviceExtension;
		p_DVCEXT->DeviceObject = ptr_PDO;
		RtlInitUnicodeString(
			&p_DVCEXT->Device_Description,
			L"This is a Buffered I/O Driver for Pseudo Device\r\n"
			L"Created by mjtsai 2003/8/1\r\n");
		IoInitializeRemoveLock(
			&p_DVCEXT->RemoveLock,
			'KCOL',
			0,
			0
		);
		p_DVCEXT->DataBuffer = ExAllocatePool(
			NonPagedPool, 1024);
		RtlZeroMemory(
			p_DVCEXT->DataBuffer,
			1024);
		//Initialize driver power state
		p_DVCEXT->SysPwrState = PowerSystemWorking;
		p_DVCEXT->DevPwrState = PowerDeviceD0;
		
		//Initialize device power information
		Global_PowerInfo_Ptr = ExAllocatePool(
			NonPagedPool, sizeof(DEVICE_POWER_INFORMATION));
		RtlZeroMemory(
			Global_PowerInfo_Ptr,
			sizeof(DEVICE_POWER_INFORMATION));
		Global_PowerInfo_Ptr->SupportQueryCapability = FALSE;
		Global_PowerInfo_Ptr->DeviceD1 = 0;
		Global_PowerInfo_Ptr->DeviceD2 = 0;
		Global_PowerInfo_Ptr->WakeFromD0 = 0;
		Global_PowerInfo_Ptr->WakeFromD1 = 0;
		Global_PowerInfo_Ptr->WakeFromD2 = 0;
		Global_PowerInfo_Ptr->WakeFromD3 = 0;
		Global_PowerInfo_Ptr->DeviceWake = 0;
		Global_PowerInfo_Ptr->SystemWake = 0;
		for (IdxPwrState = 0;
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			Global_PowerInfo_Ptr->DeviceState[IdxPwrState] = 0;
		}
		//Store next-layered device object
		//Attach device object to device stack
		p_DVCEXT->NextDeviceObject = 
			IoAttachDeviceToDeviceStack(ptr_PDO, PhysicalDeviceObject);
	}

	return status;
}

VOID 
  DriverUnload( 
    IN PDRIVER_OBJECT  DriverObject 
    )
{
	PDEVICE_EXTENSION p_DVCEXT;

	DbgPrint("In DriverUnload : Begin\r\n");

	p_DVCEXT = DriverObject->DeviceObject->DeviceExtension;
	ExFreePool(Global_PowerInfo_Ptr);
	RtlFreeUnicodeString(&Global_sz_Drv_RegInfo);
	RtlFreeUnicodeString(
		&p_DVCEXT->Device_Description);
	IoDetachDevice(
		p_DVCEXT->DeviceObject);
	IoDeleteDevice(
		p_DVCEXT->NextDeviceObject);

	DbgPrint("In DriverUnload : End\r\n");
	return;
}

NTSTATUS
  PsdoDispatchCreate(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	NTSTATUS status;

	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;
	status = IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, p_IO_STK->FileObject);
	if (NT_SUCCESS(status)) {
		CompleteRequest(Irp, STATUS_SUCCESS, 0);
		return STATUS_SUCCESS;
	} else {
		IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, p_IO_STK->FileObject);
		CompleteRequest(Irp, status, 0);
		return status;
	}
}

NTSTATUS
  PsdoDispatchClose(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;

	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;
	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, 
		p_IO_STK->FileObject);
	CompleteRequest(Irp, STATUS_SUCCESS, 0);
	return STATUS_SUCCESS;
}

NTSTATUS
  PsdoDispatchRead(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PVOID Buf;		//Buffer provided by user program
	ULONG BufLen;	//Buffer length for user provided buffer
	LONGLONG Offset;//Buffer Offset
	PVOID DataBuf;  //Buffer provided by Driver
	ULONG DataLen;  //Buffer length for Driver Data Buffer
	ULONG ByteTransferred;
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;

	DbgPrint("IRP_MJ_READ : Begin\r\n");
	//Get I/o Stack Location & Device Extension
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;

	//Get User Output Buffer & Length 
	BufLen = p_IO_STK->Parameters.Read.Length;
	Offset = p_IO_STK->Parameters.Read.ByteOffset.QuadPart;
	Buf = (PUCHAR)(Irp->AssociatedIrp.SystemBuffer) + Offset;

	//Get Driver Data Buffer & Length
	DataBuf = p_DVCEXT->DataBuffer;
	if (DataBuf == NULL)
		DataLen = 0;
	else
		DataLen = 1024;

	IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, Irp);

	DbgPrint("Output Buffer Length : %d\r\n", BufLen);
	DbgPrint("Driver Data Length : %d\r\n", DataLen);
	//
	if (BufLen <= DataLen) {
		ByteTransferred = BufLen;	
	} else {
		ByteTransferred = DataLen;
	}
	
	RtlCopyMemory(
		Buf, DataBuf, 
		ByteTransferred);

	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
	CompleteRequest(Irp, STATUS_SUCCESS, ByteTransferred);

	DbgPrint("IRP_MJ_READ : End\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS
  PsdoDispatchWrite(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	PVOID Buf;		//Buffer provided by user program
	ULONG BufLen;	//Buffer length for user provided buffer
	LONGLONG Offset;//Buffer Offset
	PVOID DataBuf;  //Buffer provided by Driver
	ULONG DataLen;  //Buffer length for Driver Data Buffer
	ULONG ByteTransferred;
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	//NTSTATUS status;

	DbgPrint("IRP_MJ_WRITE : Begin\r\n");

	//Get I/o Stack Location & Device Extension
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;

	//Get User Input Buffer & Length 
	BufLen = p_IO_STK->Parameters.Write.Length;
	Offset = p_IO_STK->Parameters.Read.ByteOffset.QuadPart;
	Buf = (PUCHAR)(Irp->AssociatedIrp.SystemBuffer) + Offset;

	//Get Driver Data Buffer & Length
	DataBuf = p_DVCEXT->DataBuffer;
	DataLen = 1024;
	
	IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, Irp);

	DbgPrint("Input Buffer Length : %d\r\n", BufLen);
	DbgPrint("Driver Data Length : %d\r\n", DataLen);

	if (BufLen <= DataLen) {
		ByteTransferred = BufLen;	
	} else {
		ByteTransferred = DataLen;
	}

	ByteTransferred = BufLen;
	RtlZeroMemory(
		p_DVCEXT->DataBuffer,
		1024);

	RtlCopyMemory(
		DataBuf,
		Buf, 
		ByteTransferred);

	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);
	CompleteRequest(Irp, STATUS_SUCCESS, ByteTransferred);

	DbgPrint("IRP_MJ_WRITE : End\r\n");
	return STATUS_SUCCESS;
}

NTSTATUS
  PsdoDispatchDeviceControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
	ULONG code, cbin, cbout, info, pwrinf_size;
	PIO_STACK_LOCATION p_IO_STK;
	PDEVICE_EXTENSION p_DVCEXT;
	//PDEVICE_POWER_INFORMATION pValue;
	ULONG IdxPwrState;
	NTSTATUS status;

	DbgPrint("IRP_MJ_DEVICE_IO_CONTROL : Begin\r\n");
	p_IO_STK = IoGetCurrentIrpStackLocation(Irp);
	p_DVCEXT = DeviceObject->DeviceExtension;
	code = p_IO_STK->Parameters.DeviceIoControl.IoControlCode;
	cbin = p_IO_STK->Parameters.DeviceIoControl.InputBufferLength;
	cbout = p_IO_STK->Parameters.DeviceIoControl.OutputBufferLength;
	IoAcquireRemoveLock(&p_DVCEXT->RemoveLock, Irp);

	switch(code)
	{
	case IOCTL_READ_DEVICE_INFO:
		if (p_DVCEXT->Device_Description.Length > cbout) {
			cbout = p_DVCEXT->Device_Description.Length;
			info = cbout;
		} else {
			info = p_DVCEXT->Device_Description.Length;
		}
			
		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
			p_DVCEXT->Device_Description.Buffer,
			info);
		status = STATUS_SUCCESS;
		break;
	case IOCTL_READ_POWER_INFO:
		pwrinf_size = sizeof(DEVICE_POWER_INFORMATION);
		if (pwrinf_size > cbout)
		{
			cbout = pwrinf_size;
			info = cbout;
		} else {
			info = pwrinf_size;
		}
		//Display Related Device Power State
		DbgPrint("Support Query Device Capability : %$r\n",
			Global_PowerInfo_Ptr->SupportQueryCapability ? "Yes" : "No");
		DbgPrint("DeviceD1 : %d\r\n", Global_PowerInfo_Ptr->DeviceD1);
		DbgPrint("DeviceD2 : %d\r\n", Global_PowerInfo_Ptr->DeviceD2);
		DbgPrint("WakeFromD0 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD0);
		DbgPrint("WakeFromD1 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD1);
		DbgPrint("WakeFromD2 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD2);
		DbgPrint("WakeFromD3 : %d\r\n", Global_PowerInfo_Ptr->WakeFromD3);
		DbgPrint("SystemWake : %d\r\n", Global_PowerInfo_Ptr->SystemWake);
		DbgPrint("DeviceWake : %d\r\n", Global_PowerInfo_Ptr->DeviceWake);
		for (IdxPwrState = 0;
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			DbgPrint("DeviceState[%d] : %d\r\n", 
				IdxPwrState, 
				Global_PowerInfo_Ptr->DeviceState[IdxPwrState]);
		}
#ifdef _DEF_HANDLE_BY_POWER_INFO_STRUCTURE
		pValue = (PDEVICE_POWER_INFORMATION)
			Irp->AssociatedIrp.SystemBuffer;
		pValue->SupportQueryCapability = Global_PowerInfo_Ptr->SupportQueryCapability;
		pValue->DeviceD1 = Global_PowerInfo_Ptr->DeviceD1;
		pValue->DeviceD2 = Global_PowerInfo_Ptr->DeviceD2;
		pValue->DeviceWake = Global_PowerInfo_Ptr->DeviceWake;
		pValue->SystemWake = Global_PowerInfo_Ptr->SystemWake;
		pValue->WakeFromD0 = Global_PowerInfo_Ptr->WakeFromD0;
		pValue->WakeFromD1 = Global_PowerInfo_Ptr->WakeFromD1;
		pValue->WakeFromD2 = Global_PowerInfo_Ptr->WakeFromD2;
		pValue->WakeFromD3 = Global_PowerInfo_Ptr->WakeFromD3;
		for (IdxPwrState = 0; 
			IdxPwrState < PowerSystemMaximum;
			IdxPwrState++)
		{
			pValue->DeviceState[IdxPwrState] = 
			Global_PowerInfo_Ptr->DeviceState[IdxPwrState];
		}
#else
		RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
			Global_PowerInfo_Ptr,
			info);
#endif
		status = STATUS_SUCCESS;
		break;
	default:
		info = 0;
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	IoReleaseRemoveLock(&p_DVCEXT->RemoveLock, Irp);

	CompleteRequest(Irp, STATUS_SUCCESS, info);
	DbgPrint("IRP_MJ_DEVICE_IO_CONTROL : End\r\n");
	return status;
}
