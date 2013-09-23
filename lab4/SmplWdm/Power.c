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

	pStk = IoGetCurrentIrpStackLocation(Irp);
	pCtx = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;
	CompleteRequest(Irp, status, 0);
	return status;
}
