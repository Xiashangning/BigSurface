#include "SurfaceButtons.hpp"

#define super IOHIDEventService

OSDefineMetaClassAndStructors(SurfaceButtons, IOHIDEventService)

VoodooGPIO* SurfaceButtons::getGPIOController() {
    VoodooGPIO* gpio_controller = NULL;

    // Wait for GPIO controller, up to 1 second
    OSDictionary* name_match = IOService::serviceMatching("VoodooGPIO");

    IOService* matched = waitForMatchingService(name_match, 1000000000);
    gpio_controller = OSDynamicCast(VoodooGPIO, matched);

    if (gpio_controller != NULL) {
        IOLog("%s::Got GPIO Controller! %s\n", getName(), gpio_controller->getName());
    }
    name_match->release();
    OSSafeReleaseNULL(matched);

    return gpio_controller;
}

IOReturn SurfaceButtons::parseButtonResources(VoodooI2CACPIResourcesParser* parser1, VoodooI2CACPIResourcesParser* parser2, VoodooI2CACPIResourcesParser* parser3) {
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (button_device->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
        !(data = OSDynamicCast(OSData, result))) {
        IOLog("%s::Could not find or evaluate _CRS method\n", getName());
        OSSafeReleaseNULL(result);
        return kIOReturnNotFound;
    }

    // There are three GPIO buttons so the CRS function returns in total 6 resources, GpioInt & GpioIo for each
    uint8_t const* crs = reinterpret_cast<uint8_t const*>(data->getBytesNoCopy());
    unsigned int length = data->getLength();
    uint32_t offset =  parser1->parseACPIResources(crs, 0, length);
    offset = parser2->parseACPIResources(crs, offset, length);
    parser3->parseACPIResources(crs, offset, length);

    OSSafeReleaseNULL(data);

    return kIOReturnSuccess;
}

IOReturn SurfaceButtons::getDeviceResources() {
    VoodooI2CACPIResourcesParser parser1, parser2, parser3;
    
    parseButtonResources(&parser1, &parser2, &parser3);

    // There is actually no way to avoid APIC interrupt if it is valid
//    if (validateAPICInterrupt() == kIOReturnSuccess)
//        return kIOReturnSuccess;

    if (parser1.found_gpio_interrupts && parser2.found_gpio_interrupts && parser3.found_gpio_interrupts) {
        IOLog("%s::Found valid GPIO interrupts\n", getName());

        // Power Button, Volume Up Button, Volume Down Button
        setProperty("powerBtnGpioPin", parser1.gpio_interrupts.pin_number, 16);
        setProperty("powerBtnGpioIRQ", parser1.gpio_interrupts.irq_type, 16);

        gpio_pin[PWBT_IDX] = parser1.gpio_interrupts.pin_number;
        gpio_irq[PWBT_IDX] = parser1.gpio_interrupts.irq_type;
        
        setProperty("volUpBtnGpioPin", parser2.gpio_interrupts.pin_number, 16);
        setProperty("volUpBtnGpioIRQ", parser2.gpio_interrupts.irq_type, 16);

        gpio_pin[VUBT_IDX] = parser2.gpio_interrupts.pin_number;
        gpio_irq[VUBT_IDX] = parser2.gpio_interrupts.irq_type;
        
        setProperty("volDownBtnGpioPin", parser3.gpio_interrupts.pin_number, 16);
        setProperty("volDownBtnGpioIRQ", parser3.gpio_interrupts.irq_type, 16);

        gpio_pin[VDBT_IDX] = parser3.gpio_interrupts.pin_number;
        gpio_irq[VDBT_IDX] = parser3.gpio_interrupts.irq_type;
        
        return kIOReturnSuccess;
    }else{
        IOLog("%s::GPIO interrupts missing!\n", getName());
        return kIOReturnError;
    }
}

void SurfaceButtons::powerInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(PWBT_IDX);
    int group = PWBT_IDX;
    // TODO
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtons::response), &group);
    
    startInterrupt(PWBT_IDX);
}

void SurfaceButtons::volumeUpInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(VUBT_IDX);
    stopInterrupt(VDBT_IDX);
    int group = VUBT_IDX;
    // TODO
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtons::response), &group);
    
    startInterrupt(VUBT_IDX);
    startInterrupt(VDBT_IDX);
}

void SurfaceButtons::volumeDownInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(VUBT_IDX);
    stopInterrupt(VDBT_IDX);
    int group = VDBT_IDX;
    // TODO
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtons::response), &group);
    
    startInterrupt(VUBT_IDX);
    startInterrupt(VDBT_IDX);
}

IOReturn SurfaceButtons::response(int *btn) {
    int number = *btn;
    if (number >= BTN_CNT)
        return kIOReturnSuccess;
    if (number == PWBT_IDX) {
        btn_status[number] = !btn_status[number]; // mannually maintain the power button status(pin status always return true)
    } else {
        btn_status[number] = gpio_controller->getPinStatus(gpio_pin[number]);
    }
    IOLog("%s::%s %s!\n", getName(), BTN_DESCRIPTION[number], btn_status[number]?"pressed":"released");
    
    OSDictionary* name_match = IOService::serviceMatching("SurfaceTypeCoverDriver");

    IOService* matched = waitForMatchingService(name_match, 100000000);
    typecover = OSDynamicCast(SurfaceTypeCoverDriver, matched);
    
    if (!typecover) {
        IOLog("%s::Could not find Surface TypeCover Driver!\n", getName());
        return kIOReturnSuccess;
    }
    name_match->release();
    OSSafeReleaseNULL(matched);
    
    AbsoluteTime timestamp;
    clock_get_uptime(&timestamp);
    if (number == PWBT_IDX && !btn_status[number]) { // releasing power button means sleeping while holding it will force the system to shutdown
        typecover->dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_Power, btn_status[number]);
    } else if (number == VUBT_IDX) {
        typecover->dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeIncrement, btn_status[number]);
    } else {
        typecover->dispatchKeyboardEvent(timestamp, kHIDPage_Consumer, kHIDUsage_Csmr_VolumeDecrement, btn_status[number]);
    }
    return kIOReturnSuccess;
}

IOService *SurfaceButtons::probe(IOService *provider, SInt32 *score){
    IOLog("%s::probing SurfaceButtons\n", getName());
    if (!super::probe(provider, score)) {
        IOLog("%s::super probe failed\n", getName());
        return nullptr;
    }
    
    button_device = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!button_device) {
        IOLog("%s::WTF? Device is not MSBT!\n", getName());
        return nullptr;
    }
    
    IOLog("%s::Microsoft ACPI button device found!\n", getName());
    
    return this;
}

bool SurfaceButtons::start(IOService *provider) {
    if (!super::start(provider)) {
        IOLog("%s::WTF? super starts failed!\n", getName());
        return false;
    }
    
    if (getDeviceResources() != kIOReturnSuccess) {
        IOLog("%s::No! Could not get GPIO infos\n", getName());
        return false;
    }

    gpio_controller = getGPIOController();
    if (!gpio_controller) {
        IOLog("%s::Could not find GPIO controller, exiting\n", getName());
        return false;
    }
    // Give the GPIO controller some time to load
    IOSleep(500);
    
    work_loop = IOWorkLoop::workLoop();

    if (!work_loop) {
        IOLog("%s Could not get work loop\n", getName());
        goto exit;
    }

    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s Could not open command gate\n", getName());
        goto exit;
    }
    
    interrupt_source[PWBT_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtons::powerInterruptOccured), this, PWBT_IDX);
    interrupt_source[VUBT_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtons::volumeUpInterruptOccured), this, VUBT_IDX);
    interrupt_source[VDBT_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtons::volumeDownInterruptOccured), this, VDBT_IDX);
    if (interrupt_source[PWBT_IDX] && interrupt_source[VUBT_IDX] && interrupt_source[VDBT_IDX]) {
        work_loop->addEventSource(interrupt_source[PWBT_IDX]);
        work_loop->addEventSource(interrupt_source[VUBT_IDX]);
        work_loop->addEventSource(interrupt_source[VDBT_IDX]);
    } else {
        IOLog("%s::Warning: Could not get interrupt event source\n", getName());
        goto exit;
    }
    
    startInterrupt(PWBT_IDX);
    startInterrupt(VUBT_IDX);
    startInterrupt(VDBT_IDX);
    
    IOLog("%s::started\n", getName());

    button_device->retain();

    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceButtons::stop(IOService *provider) {
    IOLog("%s::stopped\n", getName());
    releaseResources();
    super::stop(provider);
}

IOReturn SurfaceButtons::disableInterrupt(int source) {
    if (source == PWBT_IDX)
        return gpio_controller->disableInterrupt(gpio_pin[PWBT_IDX]);
    else if (source == VUBT_IDX)
        return gpio_controller->disableInterrupt(gpio_pin[VUBT_IDX]);
    else
        return gpio_controller->disableInterrupt(gpio_pin[VDBT_IDX]);
}

IOReturn SurfaceButtons::enableInterrupt(int source) {
    if (source == PWBT_IDX)
        return gpio_controller->enableInterrupt(gpio_pin[PWBT_IDX]);
    else if (source == VUBT_IDX)
        return gpio_controller->enableInterrupt(gpio_pin[VUBT_IDX]);
    else
        return gpio_controller->enableInterrupt(gpio_pin[VDBT_IDX]);
}

IOReturn SurfaceButtons::getInterruptType(int source, int* interrupt_type) {
    if (source == PWBT_IDX)
        return gpio_controller->getInterruptType(gpio_pin[PWBT_IDX], interrupt_type);
    else if (source == VUBT_IDX)
        return gpio_controller->getInterruptType(gpio_pin[VUBT_IDX], interrupt_type);
    else
        return gpio_controller->getInterruptType(gpio_pin[VDBT_IDX], interrupt_type);
}

IOReturn SurfaceButtons::registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) {
    if (source == PWBT_IDX) {
        gpio_controller->setInterruptTypeForPin(gpio_pin[PWBT_IDX], gpio_irq[PWBT_IDX]);
        return gpio_controller->registerInterrupt(gpio_pin[PWBT_IDX], target, handler, refcon);
    } else if (source == VUBT_IDX) {
        gpio_controller->setInterruptTypeForPin(gpio_pin[VUBT_IDX], gpio_irq[VUBT_IDX]);
        return gpio_controller->registerInterrupt(gpio_pin[VUBT_IDX], target, handler, refcon);
    } else {
        gpio_controller->setInterruptTypeForPin(gpio_pin[VDBT_IDX], gpio_irq[VDBT_IDX]);
        return gpio_controller->registerInterrupt(gpio_pin[VDBT_IDX], target, handler, refcon);
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceButtons::unregisterInterrupt(int source) {
    if (source == PWBT_IDX)
        return gpio_controller->unregisterInterrupt(gpio_pin[PWBT_IDX]);
    else if (source == VUBT_IDX)
        return gpio_controller->unregisterInterrupt(gpio_pin[VUBT_IDX]);
    else
        return gpio_controller->unregisterInterrupt(gpio_pin[VDBT_IDX]);
    return kIOReturnSuccess;
}

void SurfaceButtons::startInterrupt(int source) {
    if (is_interrupt_started[source])
        return;

    interrupt_source[source]->enable();
    
    is_interrupt_started[source] = true;
}

void SurfaceButtons::stopInterrupt(int source) {
    if (!is_interrupt_started[source])
        return;

    interrupt_source[source]->disable();
    
    is_interrupt_started[source] = false;
}


void SurfaceButtons::releaseResources() {
    if (command_gate) {
        command_gate->disable();
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    
    stopInterrupt(PWBT_IDX);
    stopInterrupt(VUBT_IDX);
    stopInterrupt(VDBT_IDX);
    
    work_loop->removeEventSource(interrupt_source[PWBT_IDX]);
    work_loop->removeEventSource(interrupt_source[VUBT_IDX]);
    work_loop->removeEventSource(interrupt_source[VDBT_IDX]);
    OSSafeReleaseNULL(interrupt_source[PWBT_IDX]);
    OSSafeReleaseNULL(interrupt_source[VUBT_IDX]);
    OSSafeReleaseNULL(interrupt_source[VDBT_IDX]);

    OSSafeReleaseNULL(work_loop);
    
    OSSafeReleaseNULL(button_device);
}
