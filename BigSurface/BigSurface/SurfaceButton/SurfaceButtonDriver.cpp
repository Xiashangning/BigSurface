//
//  SurfaceButtons.hpp
//  SurfaceButtons
//
//  Created by Xavier on 22/03/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

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

IOReturn SurfaceButtonDriver::parseButtonResources(VoodooACPIResourcesParser* parser1, VoodooACPIResourcesParser* parser2, VoodooACPIResourcesParser* parser3) {
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (acpi_device->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
        !(data = OSDynamicCast(OSData, result))) {
        IOLog("%s::Could not find or evaluate _CRS method\n", getName());
        OSSafeReleaseNULL(result);
        return kIOReturnNotFound;
    }

    // There are three GPIO buttons so the CRS function returns in total 6 resources, GpioInt & GpioIo for each
    UInt8 const* crs = reinterpret_cast<UInt8 const*>(data->getBytesNoCopy());
    unsigned int length = data->getLength();
    UInt32 offset =  parser1->parseACPIResources(crs, 0, length);
    offset = parser2->parseACPIResources(crs, offset, length);
    parser3->parseACPIResources(crs, offset, length);

    OSSafeReleaseNULL(data);

    return kIOReturnSuccess;
}

IOReturn SurfaceButtonDriver::getDeviceResources() {
    VoodooACPIResourcesParser parser1, parser2, parser3;
    
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
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&POWER_BUTTON_IDX, (void *)1);
}

void SurfaceButtonDriver::volumeUpInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    if (!awake)
        return;
    stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
    bool button_status = gpio_controller->getPinStatus(gpio_pin[VOLUME_UP_BUTTON_IDX]);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&VOLUME_UP_BUTTON_IDX, (void *)button_status);
    startInterrupt(VOLUME_DOWN_BUTTON_IDX);
}

void SurfaceButtonDriver::volumeDownInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    if (!awake)
        return;
    stopInterrupt(VOLUME_UP_BUTTON_IDX);
    bool button_status = gpio_controller->getPinStatus(gpio_pin[VOLUME_DOWN_BUTTON_IDX]);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtonDriver::response), (void *)&VOLUME_DOWN_BUTTON_IDX, (void *)button_status);
    startInterrupt(VOLUME_UP_BUTTON_IDX);
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
//    IOLog("%s::%s %s!\n", getName(), BTN_DESCRIPTION[number], btn_status[number]?"pressed":"released");
    return button_device->simulateKeyboardEvent(BTN_CMD_PAGE[number], BTN_CMD[number], btn_status[number]);
}

IOService *SurfaceButtonDriver::probe(IOService *provider, SInt32 *score){
    if (!super::probe(provider, score))
        return nullptr;
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!acpi_device)
        return nullptr;
    
    if (getDeviceResources() != kIOReturnSuccess) {
        IOLog("%s::No! Could not get GPIO infos\n", getName());
        return nullptr;
    }
    
    IOLog("%s::Surface ACPI button device found!\n", getName());
    
    return this;
}

bool SurfaceButtonDriver::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    gpio_controller = getGPIOController();
    if (!gpio_controller) {
        IOLog("%s::Could not find GPIO controller, exiting\n", getName());
        return false;
    }
    // Give the GPIO controller some time to load
    IOSleep(100);
    
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
    
    PMinit();
    acpi_device->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    button_device = OSTypeAlloc(SurfaceButtonHIDDevice);
    if (!button_device || !button_device->init() || !button_device->attach(this) || !button_device->start(this)) {
        IOLog("%s::Failed to init Surface Button HID Device!", getName());
        goto exit;
    }
    
    button_device->retain();
    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceButtonDriver::stop(IOService *provider) {
    IOLog("%s::stopped\n", getName());
    PMstop();
    releaseResources();
    super::stop(provider);
}

IOReturn SurfaceButtonDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            stopInterrupt(POWER_BUTTON_IDX);
            stopInterrupt(VOLUME_UP_BUTTON_IDX);
            stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
            IOLog("%s::Going to sleep\n", getName());
            awake = false;
        }
    } else {
        if (!awake) {
            startInterrupt(POWER_BUTTON_IDX);
            startInterrupt(VOLUME_UP_BUTTON_IDX);
            startInterrupt(VOLUME_DOWN_BUTTON_IDX);
            IOLog("%s::Woke up\n", getName());
            awake = true;
        }
    }
    return kIOPMAckImplied;
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
    if (interrupt_source[POWER_BUTTON_IDX]) {
        stopInterrupt(POWER_BUTTON_IDX);
        work_loop->removeEventSource(interrupt_source[POWER_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[POWER_BUTTON_IDX]);
    }
    if (interrupt_source[VOLUME_UP_BUTTON_IDX]) {
        stopInterrupt(VOLUME_UP_BUTTON_IDX);
        work_loop->removeEventSource(interrupt_source[VOLUME_UP_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[VOLUME_UP_BUTTON_IDX]);
    }
    if (interrupt_source[VOLUME_DOWN_BUTTON_IDX]) {
        stopInterrupt(VOLUME_DOWN_BUTTON_IDX);
        work_loop->removeEventSource(interrupt_source[VOLUME_DOWN_BUTTON_IDX]);
        OSSafeReleaseNULL(interrupt_source[VOLUME_DOWN_BUTTON_IDX]);
    }
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    OSSafeReleaseNULL(work_loop);
    
    OSSafeReleaseNULL(button_device);
}
