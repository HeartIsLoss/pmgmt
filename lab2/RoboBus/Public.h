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

DEFINE_GUID (GUID_DEVINTERFACE_RoboBus,
    0xa487b046,0xd1a3,0x4b05,0xaf,0x2e,0xb2,0xd4,0x40,0xed,0x7b,0x56);
// {a487b046-d1a3-4b05-af2e-b2d440ed7b56}

DEFINE_GUID (GUID_DEVCLASS_ROBOT,
    0x78A1C341,0x4539,0x11d3,0xB8,0x8D,0x00,0xC0,0x4F,0xAD,0x51,0x71);
//{78A1C341-4539-11d3-B88D-00C04FAD5171}