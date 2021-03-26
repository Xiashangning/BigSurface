#include "SurfaceButtons.hpp"

#define super IOService

OSDefineMetaClassAndStructors(SurfaceButtons, IOService)

bool SurfaceButtons::init(OSDictionary *dict){
    if(super::init(dict) == false){
        IOLog("%s::super init failed!", getName());
        return false;
    }
    IOLog("%s::init SurfaceButtons", getName());
    
    return true;
}

IOService *SurfaceButtons::probe(IOService *provider, SInt32 *score){
    IOLog("%s::probing SurfaceButtons", getName());
    IOService* myservice = super::probe(provider, score);
    if(!myservice){
        IOLog("%s::super probe failed", getName());
        return nullptr;
    }
    
    buttonDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    if(!buttonDevice){
        IOLog("%s::WTF Device is not MSBT!", getName());
        return nullptr;
    }
    
    IOLog("%s::----------------------------------------------SurfaceButtons found!----------------------------------------------", getName());
    
    if (getDeviceResources() != kIOReturnSuccess) {
        IOLog("%s::Could not get device resources", getName());
        return nullptr;
    }
    
    gpio_controller = getGPIOController();

    if (!gpio_controller) {
        IOLog("%s::Could not find GPIO controller, exiting", getName());
        return nullptr;
    }

//     Give the GPIO controller some time to load
    IOSleep(500);
    
    return this;
}

bool SurfaceButtons::start(IOService *provider){
    bool ret = super::start(provider);
    if(!ret){
        IOLog("%s::super starts failed!", getName());
        return ret;
    }
    IOLog("%s::started", getName());
    
    work_loop = IOWorkLoop::workLoop();

    if (!work_loop) {
        IOLog("%s::Could not get work loop", getName());
        goto exit;
    }

    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s::Could not open command gate", getName());
        goto exit;
    }
    
    buttonDevice->retain();
    gpio_controller->retain();
    
    gpio_controller->setInterruptTypeForPin(gpio_pin, gpio_irq);
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceButtons::interruptOccured), gpio_controller, gpio_pin);
    if (interrupt_source) {
        work_loop->addEventSource(interrupt_source);
    } else {
        IOLog("%s::Could not get interrupt event source", getName());
        goto exit;
    }

    interrupt_source->enable();
    gpio_controller->enableInterrupt(gpio_pin);

//    registerService();
    
    return ret;
    
exit:
    releaseResources();
    return false;
}

void SurfaceButtons::stop(IOService *provider){
    IOLog("%s::stopped", getName());
    releaseResources();
    super::stop(provider);
}

void SurfaceButtons::free(){
    IOLog("%s::free SurfaceButtons", getName());
    
    super::free();
}

IOReturn SurfaceButtons::getDeviceResources() {
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (buttonDevice->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
        !(data = OSDynamicCast(OSData, result))) {
        IOLog("%s::Could not find or evaluate _CRS method", getName());
        OSSafeReleaseNULL(result);
        return kIOReturnNotFound;
    }

    uint8_t const* crs = reinterpret_cast<uint8_t const*>(data->getBytesNoCopy());
    VoodooI2CACPICRSParser crs_parser;
    //TODO 3 GPIOInt
    crs_parser.parseACPICRS(crs, 0, data->getLength());

    data->release();
    if (crs_parser.found_gpio_interrupts) {
        setProperty("gpioPin", crs_parser.gpio_interrupts.pin_number, 16);
        setProperty("gpioIRQ", crs_parser.gpio_interrupts.irq_type, 16);

        gpio_pin = crs_parser.gpio_interrupts.pin_number;
        gpio_irq = crs_parser.gpio_interrupts.irq_type;
    } else {
        IOLog("%s::WTF? Cannot find GPIO interrupts!", getName());
        return kIOReturnNotFound;
    }

    return kIOReturnSuccess;
}

VoodooGPIO* SurfaceButtons::getGPIOController() {
    VoodooGPIO* gpio_controller = NULL;

    // Wait for GPIO controller, up to 1 second
    OSDictionary* name_match = IOService::serviceMatching("VoodooGPIO");

    IOService* matched = waitForMatchingService(name_match, 1000000000);
    gpio_controller = OSDynamicCast(VoodooGPIO, matched);
    
    name_match->release();
    OSSafeReleaseNULL(matched);
    
    if (!gpio_controller) {
        IOLog("%s::Cannot find GPIO Controller!", getName());
        return nullptr;
    }

    IOLog("%s::Got GPIO Controller!", getName());
    
    return gpio_controller;
}

void SurfaceButtons::releaseResources() {
    if (command_gate) {
        command_gate->disable();
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    
    if(interrupt_source) {
        interrupt_source->disable();
        work_loop->removeEventSource(interrupt_source);
        OSSafeReleaseNULL(interrupt_source);
    }
    gpio_controller->disableInterrupt(gpio_pin);
    gpio_controller->unregisterInterrupt(gpio_pin);
    OSSafeReleaseNULL(work_loop);
}

IOReturn SurfaceButtons::evaluateDSM(const char *uuid, UInt32 index, OSObject **result) {
    IOReturn ret;
    uuid_t guid;
    uuid_parse(uuid, guid);

    // convert to mixed-endian
    *(reinterpret_cast<uint32_t *>(guid)) = OSSwapInt32(*(reinterpret_cast<uint32_t *>(guid)));
    *(reinterpret_cast<uint16_t *>(guid) + 2) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 2));
    *(reinterpret_cast<uint16_t *>(guid) + 3) = OSSwapInt16(*(reinterpret_cast<uint16_t *>(guid) + 3));

    OSObject *params[] = {
        OSData::withBytes(guid, 16),
        OSNumber::withNumber(MSBT_DSM_REVISION, 32),
        OSNumber::withNumber(index, 32),
        OSArray::withCapacity(1),
    };

    ret = buttonDevice->evaluateObject("_DSM", result, params, 4);

    params[0]->release();
    params[1]->release();
    params[2]->release();
    params[3]->release();
    return ret;
}

void SurfaceButtons::interruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount) {
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceButtons::interruptOccuredGated));
}

void SurfaceButtons::interruptOccuredGated(){
    IOLog("%s::interruptOccured!",getName());
}
