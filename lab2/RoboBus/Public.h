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
