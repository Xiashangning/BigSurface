//
//  VoodooGPIOSunrisePointH.cpp
//  VoodooGPIO
//
//  Created by CoolStar on 8/14/17.
//  Copyright © 2017 CoolStar. All rights reserved.
//

#include "VoodooGPIOSunrisePointH.hpp"

OSDefineMetaClassAndStructors(VoodooGPIOSunrisePointH, VoodooGPIO);

bool VoodooGPIOSunrisePointH::start(IOService *provider) {
    this->pins = spth_pins;
    this->npins = ARRAY_SIZE(spth_pins);
    this->groups = spth_groups;
    this->ngroups = ARRAY_SIZE(spth_groups);
    this->functions = spth_functions;
    this->nfunctions = ARRAY_SIZE(spth_functions);
    this->communities = spth_communities;
    this->ncommunities = ARRAY_SIZE(spth_communities);

    IOLog("%s::Loading GPIO Data for SunrisePoint-H\n", getName());

    return VoodooGPIO::start(provider);
}
