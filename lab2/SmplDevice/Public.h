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

DEFINE_GUID (GUID_DEVINTERFACE_SmplDevice,
    0xf9d8cbf9,0x211e,0x4fd9,0xa3,0xc0,0xe4,0xf7,0x37,0x8a,0x31,0x04);
// {f9d8cbf9-211e-4fd9-a3c0-e4f7378a3104}
