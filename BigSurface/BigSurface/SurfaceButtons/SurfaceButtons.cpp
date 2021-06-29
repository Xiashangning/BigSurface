#include "SurfaceButtons.hpp"

#define super IOService

OSDefineMetaClassAndStructors(SurfaceButtons, IOService)

bool SurfaceButtons::init(OSDictionary *dict){
    if (super::init(dict) == false) {
        IOLog("%s::super init failed!\n", getName());
        return false;
    }
    IOLog("%s::init SurfaceButtons\n", getName());
    
    return true;
}

IOService *SurfaceButtons::probe(IOService *provider, SInt32 *score){
    IOLog("%s::probing SurfaceButtons\n", getName());
    IOService* myservice = super::probe(provider, score);
    if (!myservice) {
        IOLog("%s::super probe failed\n", getName());
        return nullptr;
    }
    
    buttonDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!buttonDevice) {
        IOLog("%s::WTF? Device is not MSBT!\n", getName());
        return nullptr;
    }
    
    IOLog("%s::----------------------------------------------SurfaceButtons found!----------------------------------------------\n", getName());
    
    OSDictionary* name_match = IOService::serviceMatching(SurfaceButtons::typecoverName);

    IOService* matched = waitForMatchingService(name_match, 1000000000);
    typecoverDevice = OSDynamicCast(SurfaceTypeCoverDriver, matched);

    if (typecoverDevice == NULL) {
        IOLog("%s::Could NOT find Surface Type Cover!", getName());
        return nullptr;
    }
    name_match->release();
    OSSafeReleaseNULL(matched);
    
    return this;
}

bool SurfaceButtons::start(IOService *provider){
    bool ret = super::start(provider);
    if (!ret) {
        IOLog("%s::super starts failed!\n", getName());
        return ret;
    }
    IOLog("%s::started\n", getName());

    buttonDevice->retain();
    typecoverDevice->retain();

    return true;
}

void SurfaceButtons::stop(IOService *provider){
    IOLog("%s::stopped\n", getName());
    super::stop(provider);
}

void SurfaceButtons::free(){
    IOLog("%s::free SurfaceButtons\n", getName());
    
    super::free();
}
