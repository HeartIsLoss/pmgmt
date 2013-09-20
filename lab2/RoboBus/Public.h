/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#pragma once


//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_RoboBus,
    0xba62137c,0x059a,0x478c,0x9e,0xcc,0xe1,0x64,0x39,0xac,0x13,0x17);
// {ba62137c-059a-478c-9ecc-e16439ac1317}


// Make sure this is the same GUID used in RoboDevice.inx
DEFINE_GUID( GUID_DEVCLASS_ROBOT, 
0x4a1b20a7, 0x3232, 0x4932, 0xb4, 0x5c, 0x8a, 0xa6, 0x41, 0x27, 0xf5, 0x25);
// {4A1B20A7-3232-4932-B45C-8AA64127F525}


#define BUS_HARDWARE_IDS L"{4A1B20A7-3232-4932-B45C-8AA64127F525}\\Robot\0"
#define BUS_HARDWARE_IDS_LENGTH sizeof (BUS_HARDWARE_IDS)



//
//  Data structure used in PlugIn and UnPlug ioctls
//

typedef struct _BUSENUM_PLUGIN_HARDWARE
{
    IN ULONG Size;
    IN ULONG SerialNo;
#pragma warning(push)
#pragma warning(disable:4200)
	IN wchar_t HardwareIDs[];
#pragma warning(pop)
} BUSENUM_PLUGIN_HARDWARE;


typedef struct _BUSENUM_UNPLUG_HARDWARE
{
    IN ULONG Size;
    ULONG   SerialNo;
    ULONG Reserved[2];
} BUSENUM_UNPLUG_HARDWARE;


typedef struct _BUSENUM_EJECT_HARDWARE
{
    IN ULONG Size;
    ULONG   SerialNo;
    ULONG Reserved[2];
} BUSENUM_EJECT_HARDWARE;


#define FILE_DEVICE_BUSENUM         FILE_DEVICE_BUS_EXTENDER
#define BUSENUM_IOCTL(index)		CTL_CODE (FILE_DEVICE_BUSENUM, index, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_BUSENUM_PLUGIN_HARDWARE               BUSENUM_IOCTL (0x0)
#define IOCTL_BUSENUM_UNPLUG_HARDWARE               BUSENUM_IOCTL (0x1)
#define IOCTL_BUSENUM_EJECT_HARDWARE                BUSENUM_IOCTL (0x2)
#define IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE     BUSENUM_IOCTL (0x3)

