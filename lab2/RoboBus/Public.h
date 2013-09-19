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
    0xba62137c,0x059a,0x478c,0x9e,0xcc,0xe1,0x64,0x39,0xac,0x13,0x17);
// {ba62137c-059a-478c-9ecc-e16439ac1317}
