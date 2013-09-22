#include <wdm.h>
#include "Driver.h"

NTSTATUS CompleteRequest(
         IN PIRP Irp,
         IN NTSTATUS status,
         IN ULONG_PTR info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
