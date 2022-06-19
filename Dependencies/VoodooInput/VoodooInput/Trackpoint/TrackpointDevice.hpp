/*
 * TrackpointDevice.hpp
 * VoodooTrackpoint
 *
 * Copyright (c) 2019 Leonard Kleinhans <leo-labs>
 *
 */

#ifndef TrackpointDevice_hpp
#define TrackpointDevice_hpp

#include <IOKit/hidsystem/IOHIPointing.h>
#include <IOKit/hidsystem/IOHIDParameter.h>

class TrackpointDevice : public IOHIPointing {
    typedef IOHIPointing super;
    OSDeclareDefaultStructors(TrackpointDevice);
protected:
    virtual IOItemCount buttonCount() override;
    virtual IOFixed resolution() override;
    
    
public:
    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    
    virtual UInt32 deviceType() override;
    virtual UInt32 interfaceID() override;
    
    void updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp);
    void updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp);

};

#endif /* TrackpointDevice_hpp */
