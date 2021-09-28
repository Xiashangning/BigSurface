#include "SurfaceButtonDriver.hpp"

#define super IOHIDEventService

OSDefineMetaClassAndStructors(SurfaceButtonDriver, IOHIDEventService)

VoodooGPIO* SurfaceButtonDriver::getGPIOController() {
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

IOReturn SurfaceButtonDriver::parseButtonResources(VoodooI2CACPIResourcesParser* parser1, VoodooI2CACPIResourcesParser* parser2, VoodooI2CACPIResourcesParser* parser3) {
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (acpi_device->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
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

IOReturn SurfaceButtonDriver::getDeviceResources() {
    VoodooI2CACPIResourcesParser parser1, parser2, parser3;
    
    parseButtonResources(&parser1, &parser2, &parser3);

    if (parser1.found_gpio_interrupts && parser2.found_gpio_interrupts && parser3.found_gpio_interrupts) {
        IOLog("%s::Found valid GPIO interrupts\n", getName());

        // Power Button, Volume Up Button, Volume Down Button
        setProperty("powerBtnGpioPin", parser1.gpio_interrupts.pin_number, 16);
        setProperty("powerBtnGpioIRQ", parser1.gpio_interrupts.irq_type, 16);

        gpio_pin[POWER_BUTTON_IDX] = parser1.gpio_interrupts.pin_number;
        gpio_irq[POWER_BUTTON_IDX] = parser1.gpio_interrupts.irq_type;
        
        setProperty("volUpBtnGpioPin", parser2.gpio_interrupts.pin_number, 16);
        setProperty("volUpBtnGpioIRQ", parser2.gpio_interrupts.irq_type, 16);

        gpio_pin[VOLUME_UP_BUTTON_IDX] = parser2.gpio_interrupts.pin_number;
        gpio_irq[VOLUME_UP_BUTTON_IDX] = parser2.gpio_interrupts.irq_type;
        
        setProperty("volDownBtnGpioPin", parser3.gpio_interrupts.pin_number, 16);
        setProperty("volDownBtnGpioIRQ", parser3.gpio_interrupts.irq_type, 16);

        gpio_pin[VOLUME_DOWN_BUTTON_IDX] = parser3.gpio_interrupts.pin_number;
        gpio_irq[VOLUME_DOWN_BUTTON_IDX] = parser3.gpio_interrupts.irq_type;
        
        return kIOReturnSuccess;
    }else{
        IOLog("%s::GPIO interrupts missing!\n", getName());
        return kIOReturnError;
    }
}

void SurfaceButtonDriver::powerInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(POWER_BUTTON_IDX);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&POWER_BUTTON_IDX, (void *)1);
    startInterrupt(POWER_BUTTON_IDX);
}

void SurfaceButtonDriver::volumeUpInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(VOLUME_UP_BUTTON_IDX);
    stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
    bool button_status = gpio_controller->getPinStatus(gpio_pin[VOLUME_UP_BUTTON_IDX]);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&VOLUME_UP_BUTTON_IDX, (void *)button_status);
    startInterrupt(VOLUME_UP_BUTTON_IDX);
    startInterrupt(VOLUME_DOWN_BUTTON_IDX);
}

void SurfaceButtonDriver::volumeDownInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    stopInterrupt(VOLUME_UP_BUTTON_IDX);
    stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
    bool button_status = gpio_controller->getPinStatus(gpio_pin[VOLUME_DOWN_BUTTON_IDX]);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&VOLUME_DOWN_BUTTON_IDX, (void *)button_status);
    startInterrupt(VOLUME_UP_BUTTON_IDX);
    startInterrupt(VOLUME_DOWN_BUTTON_IDX);
}

IOReturn SurfaceButtonDriver::response(int* btn, void* status) {
    int number = *btn;
    bool pressed = !!status;
    if (number >= BTN_CNT)
        return kIOReturnSuccess;
    if (number == POWER_BUTTON_IDX)
        btn_status[number] = !btn_status[number]; // mannually maintain the power button status(pin status always return true)
     else
        btn_status[number] = pressed;
    //IOLog("%s::%s %s!\n", getName(), BTN_DESCRIPTION[number], btn_status[number]?"pressed":"released");
    return button_device->simulateKeyboardEvent(BTN_CMD_PAGE[number], BTN_CMD[number], btn_status[number]);
}

IOService *SurfaceButtonDriver::probe(IOService *provider, SInt32 *score){
    IOLog("%s::probing SurfaceButtons\n", getName());
    if (!super::probe(provider, score)) {
        IOLog("%s::super probe failed\n", getName());
        return nullptr;
    }
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!acpi_device) {
        IOLog("%s::WTF? Device is not MSBT!\n", getName());
        return nullptr;
    }
    
    IOLog("%s::Microsoft ACPI button device found!\n", getName());
    
    return this;
}

bool SurfaceButtonDriver::start(IOService *provider) {
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
    
    interrupt_source[POWER_BUTTON_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtonDriver::powerInterruptOccured), this, POWER_BUTTON_IDX);
    interrupt_source[VOLUME_UP_BUTTON_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtonDriver::volumeUpInterruptOccured), this, VOLUME_UP_BUTTON_IDX);
    interrupt_source[VOLUME_DOWN_BUTTON_IDX] = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtonDriver::volumeDownInterruptOccured), this, VOLUME_DOWN_BUTTON_IDX);
    if (interrupt_source[POWER_BUTTON_IDX] && interrupt_source[VOLUME_UP_BUTTON_IDX] && interrupt_source[VOLUME_DOWN_BUTTON_IDX]) {
        work_loop->addEventSource(interrupt_source[POWER_BUTTON_IDX]);
        work_loop->addEventSource(interrupt_source[VOLUME_UP_BUTTON_IDX]);
        work_loop->addEventSource(interrupt_source[VOLUME_DOWN_BUTTON_IDX]);
    } else {
        IOLog("%s::Warning: Could not get interrupt event source\n", getName());
        goto exit;
    }
    
    startInterrupt(POWER_BUTTON_IDX);
    startInterrupt(VOLUME_UP_BUTTON_IDX);
    startInterrupt(VOLUME_DOWN_BUTTON_IDX);
    
    button_device = new SurfaceButtonHIDDevice;

    if (!button_device || !button_device->init() || !button_device->attach(this) || !button_device->start(this)) {
        IOLog("%s::Failed to init Surface Button HID Device!", getName());
        goto exit;
    }
    
    IOLog("%s::started\n", getName());

    acpi_device->retain();
    button_device->retain();

    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceButtonDriver::stop(IOService *provider) {
    IOLog("%s::stopped\n", getName());
    releaseResources();
    super::stop(provider);
}

IOReturn SurfaceButtonDriver::disableInterrupt(int source) {
    return gpio_controller->disableInterrupt(gpio_pin[source]);
}

IOReturn SurfaceButtonDriver::enableInterrupt(int source) {
    return gpio_controller->enableInterrupt(gpio_pin[source]);
}

IOReturn SurfaceButtonDriver::getInterruptType(int source, int* interrupt_type) {
    return gpio_controller->getInterruptType(gpio_pin[source], interrupt_type);
}

IOReturn SurfaceButtonDriver::registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) {
    gpio_controller->setInterruptTypeForPin(gpio_pin[source], gpio_irq[source]);
    return gpio_controller->registerInterrupt(gpio_pin[source], target, handler, refcon);
}

IOReturn SurfaceButtonDriver::unregisterInterrupt(int source) {
    return gpio_controller->unregisterInterrupt(gpio_pin[source]);
}

void SurfaceButtonDriver::startInterrupt(int source) {
    if (is_interrupt_started[source])
        return;

    interrupt_source[source]->enable();
    
    is_interrupt_started[source] = true;
}

void SurfaceButtonDriver::stopInterrupt(int source) {
    if (!is_interrupt_started[source])
        return;

    interrupt_source[source]->disable();
    
    is_interrupt_started[source] = false;
}


void SurfaceButtonDriver::releaseResources() {
    if (command_gate) {
        command_gate->disable();
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    
    stopInterrupt(POWER_BUTTON_IDX);
    stopInterrupt(VOLUME_UP_BUTTON_IDX);
    stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
    
    if (work_loop) {
        work_loop->removeEventSource(interrupt_source[POWER_BUTTON_IDX]);
        work_loop->removeEventSource(interrupt_source[VOLUME_UP_BUTTON_IDX]);
        work_loop->removeEventSource(interrupt_source[VOLUME_DOWN_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[POWER_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[VOLUME_UP_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[VOLUME_DOWN_BUTTON_IDX]);
        OSSafeReleaseNULL(work_loop);
    }
    
    OSSafeReleaseNULL(acpi_device);
    OSSafeReleaseNULL(button_device);
}
