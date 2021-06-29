//
//  SurfaceButtons.hpp
//  SurfaceButtons
//
//  Created by Xia on 22/03/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceButtons_hpp
#define SurfaceButtons_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../SurfaceTypeCover/SurfaceTypeCoverDriver.hpp"

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

class EXPORT SurfaceButtons : public IOService {
    OSDeclareDefaultStructors(SurfaceButtons);
    
private:
    IOACPIPlatformDevice *buttonDevice {nullptr};
    
    static constexpr const char *typecoverName = "SurfaceTypeCoverDriver";
    SurfaceTypeCoverDriver *typecoverDevice {nullptr};
    
public:
    
    bool init(OSDictionary* dict) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    void free(void) override;
    
private:
};

#endif
