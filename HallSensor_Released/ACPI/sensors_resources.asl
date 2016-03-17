//===========================================================================
//                           <sensors_resources.asl>
// DESCRIPTION
//   This file contans the resources needed by sensor drivers.
//
//
//   Copyright (c) 2010-2014 by QUALCOMM Technologies, Inc.  All Rights Reserved.
//   Qualcomm Confidential and Proprietary
//
//===========================================================================

Scope(\_SB.PEP0)
{
    Method(NPMX)
    {
        Return(NPXC)
    }

    Name(NPXC, Package()
    {
		//Hall sensor "Sensors_Resource.asl"
        Package()
        {
            "DEVICE",
            "\\_SB.Hall",
            Package()
            {
                "DSTATE",
                0,
                package()
                {
                    "PMICVREGVOTE",
                    package()
                    {
                        "PPP_RESOURCE_ID_LDO6_A", // VREG ID
                        1,          // Voltage Regulator type = LDO
                        1800000,    // Voltage in micro volts
                        100,        // Peak current in microamps
                        1,          // force enable from software
                        0,          // disable pin control enable
                        1,          // power mode - Normal Power Mode
                        0,          // power mode pin control - disable
                        0,          // bypass mode allowed
                        0,          // head room voltage
                    },
                },
            },
            Package() {"DSTATE", 0x1}, // D1 state
            Package() {"DSTATE", 0x2}, // D2 state
            Package()
            {
                "DSTATE",
                3,
                package()
                {
                    "PMICVREGVOTE",
                    package()
                    {
                        "PPP_RESOURCE_ID_LDO6_A", // VREG ID
                        1,          // Voltage Regulator type = LDO
                        0,          // Voltage in micro volts
                        0,          // Peak current in microamps
                        0,          // force enable from software
                        0,          // disable pin control enable
                        0,          // power mode - Normal Power Mode
                        0,          // power mode pin control - disable
                        0,          // bypass mode allowed
                        0,          // head room voltage
                    },
                },
            },
        },
    })
}
