//
// Hall Sensor "Sensors.als"  need include into Dsdt.asl
//
Device (Hall)
{
    Name (_HID, "HallSensor")            
    Name (_UID, 1)            
                
    Name (_DEP, Package(0x2)            
    {            
        \_SB_.PEP0,            
                    \_SB_.GIO0            
    })            
                
    Method (_S1D, 0) { Return (3) }     // S1 => D3
    Method (_S2D, 0) { Return (3) }     // S2 => D3
    Method (_S3D, 0) { Return (3) }     // S3 => D3
                
    Method (_CRS, 0x0, NotSerialized) {
        Name (RBUF, ResourceTemplate ()
        {       
            //polling way                    
            //GpioIO(Exclusive, PullNone, 0, 0, , "\\_SB.GIO0",) {124}                      
                                
            //Interrupt way                    
            GpioInt(Edge, ActiveBoth,  SharedandWake, PullNone, 0, "\\_SB.GIO0", ,) {124}                    
            GpioIo(Shared, PullNone, 0, 0, , "\\_SB.GIO0", ,) {124}                    

        })
        Return (RBUF)
    }
}