//
//  VoodooI2CHIDTransducerWrapper.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 02/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDTransducerWrapper_hpp
#define VoodooI2CHIDTransducerWrapper_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>

#include <IOKit/hid/IOHIDElement.h>

#include "../Multitouch Support/VoodooI2CDigitiserTransducer.hpp"

class EXPORT VoodooI2CHIDTransducerWrapper : public OSObject {
  OSDeclareDefaultStructors(VoodooI2CHIDTransducerWrapper);

 public:
    OSArray*      transducers;
    
    IOHIDElement* first_identifier;

    bool init() override;
    void free() override;

    static VoodooI2CHIDTransducerWrapper* wrapper();
};


#endif /* VoodooI2CHIDTransducerWrapper_hpp */
