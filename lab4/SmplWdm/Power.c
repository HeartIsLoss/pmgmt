#include <wdm.h>
#include "Driver.h"


NTSTATUS
SwdmDispatchPower(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp
    )
{
    PIO_STACK_LOCATION pStk;
	PDEVICE_EXTENSION pCtx;
	NTSTATUS status = STATUS_SUCCESS;
    ULONG MinorFunction;

	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;

    MinorFunction = pStk->MinorFunction;
    IoAcquireRemoveLock( &pCtx->RemoveLock, NULL );

    switch( MinorFunction )
    {
    case IRP_MN_SET_POWER:
        break;
        
    case IRP_MN_QUERY_POWER:
        break;

    case IRP_MN_WAIT_WAKE:
        break;

    case IRP_MN_POWER_SEQUENCE:
        break;

    };
    
    IoReleaseRemoveLock( &pCtx->RemoveLock, NULL );
    CompleteRequest(Irp, status, 0);
	return status;
}
