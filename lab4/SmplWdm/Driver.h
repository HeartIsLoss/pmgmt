#pragma once


typedef struct tagDEVICE_EXTENSION
{
	PDEVICE_OBJECT DeviceObject;		// device object this driver creates
	PDEVICE_OBJECT NextDeviceObject;	// next-layered device object in this device stack
	DEVICE_CAPABILITIES pdc;		// device capability
    PNP_BUS_INFORMATION* pBusInfo;
	IO_REMOVE_LOCK RemoveLock;		// removal control locking structure
	LONG handles;				// # open handles
	PVOID DataBuffer;                       // Internal Buffer for Read/Write I/O
	UNICODE_STRING Device_Description;	// Device Description
        SYSTEM_POWER_STATE SysPwrState;		// Current System Power State
        DEVICE_POWER_STATE DevPwrState;		// Current Device Power State
	PIRP PowerIrp;				// Current Handling Power-Related IRP
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/*
Device Power State Structure for Device Capability Handling
*/
typedef struct tagDEVICE_POWER_INFORMATION {
  BOOLEAN   SupportQueryCapability;
  ULONG  DeviceD1;
  ULONG  DeviceD2;
  ULONG  WakeFromD0;
  ULONG  WakeFromD1;
  ULONG  WakeFromD2;
  ULONG  WakeFromD3;
  SYSTEM_POWER_STATE  SystemWake;
  DEVICE_POWER_STATE  DeviceWake;
  DEVICE_POWER_STATE  DeviceState[PowerSystemMaximum];
} DEVICE_POWER_INFORMATION, *PDEVICE_POWER_INFORMATION;


#define IOCTL_READ_DEVICE_INFO              \
        CTL_CODE(                           \
                    FILE_DEVICE_UNKNOWN,    \
                    0x800,                  \
                    METHOD_BUFFERED,        \
                    FILE_ANY_ACCESS)
                    
//I/O Control Code for Power Information retrieval
#define IOCTL_READ_POWER_INFO	          \
	CTL_CODE(			  \
		 FILE_DEVICE_UNKNOWN,	  \
		 0x801,			  \
		 METHOD_BUFFERED,	  \
		 FILE_ANY_ACCESS)

NTSTATUS
  CompleteRequest(
		IN PIRP Irp,
		IN NTSTATUS status,
		IN ULONG_PTR info);


// --------------------------- Driver.c

NTSTATUS 
  DriverEntry( 
    IN PDRIVER_OBJECT  DriverObject, 
    IN PUNICODE_STRING  RegistryPath
	);

NTSTATUS
  AddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject 
    );

VOID 
  DriverUnload( 
    IN PDRIVER_OBJECT  DriverObject 
    ); 

NTSTATUS
  PsdoDispatchCreate(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  PsdoDispatchClose(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  PsdoDispatchRead(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  PsdoDispatchWrite(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  PsdoDispatchDeviceControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );



// ------------------------------------- PM

NTSTATUS
  PsdoDispatchPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  CompletionQuerySystemPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    );

NTSTATUS
  CompletionQueryDevicePower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    );

NTSTATUS
  CompletionSetSystemPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    );

NTSTATUS
  CompletionDevicePowerUp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    );

VOID
  CallBackForRequestPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

VOID
  CallBackForSetPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );


// -------------------------------------------------------------------

NTSTATUS
  PsdoDispatchPnP(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  StartDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  StopDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  RemoveDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  QueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    );

NTSTATUS
  CompletionQueryCapability(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context
    );

NTSTATUS QueryBus( PDEVICE_OBJECT DeviceObject, PIRP Irp );

