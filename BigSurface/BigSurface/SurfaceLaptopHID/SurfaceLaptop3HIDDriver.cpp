//
//  SurfaceLaptop3HIDDriver.cpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/16.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceLaptop3HIDDriver.hpp"
#include "SurfaceLaptopKeyboard.hpp"
#include "SurfaceLaptopTouchpad.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super IOService
OSDefineMetaClassAndStructors(SurfaceLaptop3HIDDriver, IOService)

void SurfaceLaptop3HIDDriver::eventReceived(SurfaceLaptop3Nub *sender, SurfaceLaptop3HIDDeviceType device, UInt8 *buffer, UInt16 len) {
    switch (device) {
        case SurfaceLaptop3Keyboard:
            memcpy(kbd_report, buffer, len);
            kbd_report_len = len;
            kbd_interrupt->interruptOccurred(nullptr, this, 0);
            break;
        case SurfaceLaptop3Touchpad:
            memcpy(tpd_report, buffer, len);
            tpd_report_len = len;
            tpd_interrupt->interruptOccurred(nullptr, this, 0);
            break;
        default:
            LOG("WTF? Unknown event type");
            break;
    }
}

void SurfaceLaptop3HIDDriver::keyboardInputReceived(IOInterruptEventSource *sender, int count) {
    if (!awake)
        return;
    
    IOBufferMemoryDescriptor *report = IOBufferMemoryDescriptor::withBytes(kbd_report, kbd_report_len, kIODirectionNone);
    if (keyboard->handleReport(report) != kIOReturnSuccess)
        LOG("Handle keyboard report error!");
    
    OSSafeReleaseNULL(report);
}

void SurfaceLaptop3HIDDriver::touchpadInputReceived(IOInterruptEventSource *sender, int count) {
    if (!awake)
        return;
    
    IOBufferMemoryDescriptor *report = IOBufferMemoryDescriptor::withBytes(tpd_report, tpd_report_len, kIODirectionNone);
    if (touchpad->handleReport(report) != kIOReturnSuccess)
        LOG("Handle touchpad report error!");

    OSSafeReleaseNULL(report);
}

bool SurfaceLaptop3HIDDriver::init(OSDictionary *properties) {
    if (!super::init(properties))
        return false;
    
    memset(kbd_report, 0, sizeof(kbd_report));
    memset(tpd_report, 0, sizeof(tpd_report));
    return true;
}

IOService *SurfaceLaptop3HIDDriver::probe(IOService *provider, SInt32 *score) {
	if (!super::probe(provider, score))
        return nullptr;

    nub = OSDynamicCast(SurfaceLaptop3Nub, provider);
    if (!nub)
        return nullptr;
    
	return this;
}

bool SurfaceLaptop3HIDDriver::start(IOService *provider) {
	if (!super::start(provider))
		return false;

    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not create work loop!");
        goto exit;
    }
    
    kbd_interrupt = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceLaptop3HIDDriver::keyboardInputReceived));
    if (!kbd_interrupt) {
        LOG("Could not create keyboard interrupt event!");
        goto exit;
    }
    work_loop->addEventSource(kbd_interrupt);
    
    tpd_interrupt = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceLaptop3HIDDriver::touchpadInputReceived));
    if (!tpd_interrupt) {
        LOG("Could not create touchpad interrupt event!");
        goto exit;
    }
    work_loop->addEventSource(tpd_interrupt);
    
    if (nub->registerHIDEvent(this, OSMemberFunctionCast(SurfaceLaptop3Nub::EventHandler, this, &SurfaceLaptop3HIDDriver::eventReceived)) != kIOReturnSuccess) {
        LOG("HID event registration failed!");
        goto exit;
    }
    
    // create keyboard & touchpad devices
    keyboard = OSTypeAlloc(SurfaceLaptopKeyboard);
    if (!keyboard || !keyboard->init() || !keyboard->attach(this)) {
        LOG("Could not init Surface Laptop3 keyboard device");
        goto exit;
    }
    if (!keyboard->start(this)) {
        keyboard->detach(this);
        LOG("Could not start Surface Laptop3 keyboard device");
        goto exit;
    }
    
    touchpad = OSTypeAlloc(SurfaceLaptopTouchpad);
    if (!touchpad || !touchpad->init() || !touchpad->attach(this)) {
        LOG("Could not init Surface Laptop3 touchpad device");
        goto exit;
    }
    if (!touchpad->start(this)) {
        touchpad->detach(this);
        LOG("Could not start Surface Laptop3 touchpad device");
        goto exit;
    }
    
    PMinit();
    nub->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
	return true;
exit:
    releaseResources();
    return false;
}

void SurfaceLaptop3HIDDriver::releaseResources() {
    nub->unregisterHIDEvent(this);
    if (kbd_interrupt) {
        kbd_interrupt->disable();
        work_loop->removeEventSource(kbd_interrupt);
        OSSafeReleaseNULL(kbd_interrupt);
    }
    if (tpd_interrupt) {
        tpd_interrupt->disable();
        work_loop->removeEventSource(tpd_interrupt);
        OSSafeReleaseNULL(tpd_interrupt);
    }
    OSSafeReleaseNULL(work_loop);
    
    if (keyboard) {
        keyboard->stop(this);
        keyboard->detach(this);
        OSSafeReleaseNULL(keyboard);
    }
    if (touchpad) {
        touchpad->stop(this);
        touchpad->detach(this);
        OSSafeReleaseNULL(touchpad);
    }
}

void SurfaceLaptop3HIDDriver::stop(IOService *provider) {
    PMstop();
    releaseResources();
    super::stop(provider);
}

void SurfaceLaptop3HIDDriver::free() {
    super::free();
}

IOReturn SurfaceLaptop3HIDDriver::setPowerState(unsigned long whichState, IOService *device) {
    if (device != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
            kbd_interrupt->disable();
            tpd_interrupt->disable();
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            kbd_interrupt->enable();
            tpd_interrupt->enable();
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

IOReturn SurfaceLaptop3HIDDriver::getKeyboardDescriptor(SurfaceHIDDescriptor *desc) {
    return nub->getHIDDescriptor(SurfaceLaptop3Keyboard, desc);
}

IOReturn SurfaceLaptop3HIDDriver::getKeyboardAttributes(SurfaceHIDAttributes *attr) {
    return nub->getHIDAttributes(SurfaceLaptop3Keyboard, attr);
}

IOReturn SurfaceLaptop3HIDDriver::getKeyboardReportDescriptor(UInt8 *buffer, UInt16 len) {
    return nub->getHIDReportDescriptor(SurfaceLaptop3Keyboard, buffer, len);
}

IOReturn SurfaceLaptop3HIDDriver::getKeyboardRawReport(UInt8 report_id, UInt8 *buffer, UInt16 len) {
    return nub->getHIDRawReport(SurfaceLaptop3Keyboard, report_id, buffer, len);
}

void SurfaceLaptop3HIDDriver::setKeyboardRawReport(UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len) {
    nub->setHIDRawReport(SurfaceLaptop3Keyboard, report_id, feature, buffer, len);
}

IOReturn SurfaceLaptop3HIDDriver::getTouchpadDescriptor(SurfaceHIDDescriptor *desc) {
    return nub->getHIDDescriptor(SurfaceLaptop3Touchpad, desc);
}

IOReturn SurfaceLaptop3HIDDriver::getTouchpadAttributes(SurfaceHIDAttributes *attr) {
    return nub->getHIDAttributes(SurfaceLaptop3Touchpad, attr);
}

IOReturn SurfaceLaptop3HIDDriver::getTouchpadReportDescriptor(UInt8 *buffer, UInt16 len) {
    return nub->getHIDReportDescriptor(SurfaceLaptop3Touchpad, buffer, len);
}

IOReturn SurfaceLaptop3HIDDriver::getTouchpadRawReport(UInt8 report_id, UInt8 *buffer, UInt16 len) {
    return nub->getHIDRawReport(SurfaceLaptop3Touchpad, report_id, buffer, len);
}

void SurfaceLaptop3HIDDriver::setTouchpadRawReport(UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len) {
    nub->setHIDRawReport(SurfaceLaptop3Touchpad, report_id, feature, buffer, len);
}

