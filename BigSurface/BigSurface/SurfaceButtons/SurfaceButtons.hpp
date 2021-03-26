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
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOInterruptEventSource.h>

#include "../../Dependencies/VoodooGPIO/VoodooGPIO/VoodooGPIO.hpp"
#include "../../Dependencies/VoodooI2CACPICRSParser/VoodooI2CACPICRSParser.hpp"

#define MSBT_DSM_UUID "6fd05c69-cde3-49f4-95ed-ab1665498035"
#define MSBT_DSM_REVISION 1

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

class EXPORT SurfaceButtons : public IOService {
    OSDeclareDefaultStructors(SurfaceButtons);
    
private:
    IOACPIPlatformDevice *buttonDevice {nullptr};
    
    IOWorkLoop* work_loop {nullptr};
    IOCommandGate* command_gate;
    
    IOInterruptEventSource* interrupt_source;

    VoodooGPIO* gpio_controller;
    int gpio_irq;
    UInt16 gpio_pin;
    
public:    
    /* Evaluate _DSM for specific GUID and function index. Assume Revision ID is 1 for now.
     * @uuid Human-readable GUID string (big-endian)
     * @index Function index
     * @result The return is a buffer containing one bit for each function index if Function Index is zero, otherwise could be any data object (See 9.1.1 _DSM (Device Specific Method) in ACPI Specification, Version 6.3)
     *
     * @return *kIOReturnSuccess* upon a successfull *_DSM*(*XDSM*) parse, otherwise failed when executing *evaluateObject*.
     */

    IOReturn evaluateDSM(const char *uuid, UInt32 index, OSObject **result);
    
    bool init(OSDictionary* dict) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    void free(void) override;
    
private:
    /* Instantiates a <VoodooI2CACPICRSParser> object to grab I2C slave properties as well as potential GPIO interrupt properties.
     *
     * @return *kIOReturnSuccess* upon a successfull *_CRS* parse, *kIOReturnNotFound* if no I2C slave properties were found.
     */

    IOReturn getDeviceResources();
    
    /* Releases resources allocated in <start>
     *
     * This function is called during a graceful exit from <start> and during
     * execution of <stop> in order to release resources retained by <start>.
     */

    void releaseResources();

    /* Searches the IOService plane to find a <VoodooGPIO> controller object.
     */

    VoodooGPIO* getGPIOController();
    
    void interruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);
    
    void interruptOccuredGated();
};

#endif
