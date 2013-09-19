/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that app can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_RoboDevice,
    0x81a13f87,0xab33,0x4dac,0xb1,0x0e,0x8b,0xd5,0xa4,0xe7,0xf9,0x6f);
// {81a13f87-ab33-4dac-b10e-8bd5a4e7f96f}
