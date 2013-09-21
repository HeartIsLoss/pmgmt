#include "driver.h"
#include "Public.h"


NTSTATUS RoboBusCreatePdo( WDFDEVICE device, wchar_t* HardwareIds, ULONG SerialNo )
{
	PWDFDEVICE_INIT pDeviceInit = NULL;
	NTSTATUS status;
	PDO_DEVICE_CONTEXT* pdoContext = NULL;
	WDFDEVICE hchild;
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_DEVICE_PNP_CAPABILITIES pnpCaps;
	WDF_DEVICE_POWER_CAPABILITIES powerCaps;
	UNICODE_STRING deviceId;
	DECLARE_CONST_UNICODE_STRING( deviceLocation, L"Robot Bus 0" );
    DECLARE_UNICODE_STRING_SIZE( buffer, 80 );

    PAGED_CODE();

	DbgPrint( "RoboBusCreatePdo: device=%p, SerialNo=%d\n", device, SerialNo );


	pDeviceInit = WdfPdoInitAllocate( device );
	if( pDeviceInit == NULL ) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}

	WdfDeviceInitSetDeviceType( pDeviceInit, FILE_DEVICE_BUS_EXTENDER );
	RtlInitUnicodeString( &deviceId, HardwareIds );
	status = WdfPdoInitAssignDeviceID( pDeviceInit, &deviceId );
    status = WdfPdoInitAddHardwareID( pDeviceInit, &deviceId );
    status =  RtlUnicodeStringPrintf( &buffer, L"%02d", SerialNo );
	status = WdfPdoInitAssignInstanceID( pDeviceInit, &buffer );
    status = RtlUnicodeStringPrintf( &buffer, L"Uri_London_Robot_%02d", SerialNo );
    status = WdfPdoInitAddDeviceText( pDeviceInit, &buffer, &deviceLocation, 0x409 );
    WdfPdoInitSetDefaultLocale( pDeviceInit, 0x409 );
    WdfDeviceInitSetPowerPolicyOwnership( pDeviceInit, TRUE );

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE( &attributes, PDO_DEVICE_CONTEXT );

    status = WdfDeviceCreate( &pDeviceInit, &attributes, &hchild );

	pdoContext = getPdoContext( hchild );
	pdoContext->SerialNo = SerialNo;

    WDF_DEVICE_PNP_CAPABILITIES_INIT( &pnpCaps );
    pnpCaps.Removable = WdfTrue;
    pnpCaps.EjectSupported = WdfTrue;
    pnpCaps.SurpriseRemovalOK = WdfTrue;
    pnpCaps.Address = SerialNo;
    pnpCaps.UINumber = SerialNo;
    WdfDeviceSetPnpCapabilities( hchild, &pnpCaps );

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&powerCaps);
    powerCaps.DeviceD1 = WdfTrue;
    powerCaps.WakeFromD1 = WdfTrue;
    powerCaps.DeviceWake = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemWorking]   = PowerDeviceD0;
    powerCaps.DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
    powerCaps.DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    powerCaps.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
    powerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    powerCaps.DeviceState[PowerSystemShutdown] = PowerDeviceD3;
    WdfDeviceSetPowerCapabilities( hchild, &powerCaps );

	status = WdfFdoAddStaticChild( device, hchild );

	if( NT_SUCCESS(status) ) {
		DbgPrintEx( DPFLTR_IHVBUS_ID, DPFLTR_ERROR_LEVEL, "RoboBusCreatePdo failed 0x%x\n", status );

		if( pDeviceInit != NULL ) {
			WdfDeviceInitFree( pDeviceInit );
		}

		if( hchild ) {
			WdfObjectDelete( hchild );
		}
	}

	return status;
}

