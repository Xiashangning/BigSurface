//
//  VoodooGPIOSunrisePointLP.cpp
//  VoodooGPIO
//
//  Created by CoolStar on 8/14/17.
//  Copyright © 2017 CoolStar. All rights reserved.
//

#include "VoodooGPIOSunrisePointLP.hpp"

OSDefineMetaClassAndStructors(VoodooGPIOSunrisePointLP, VoodooGPIO);

bool VoodooGPIOSunrisePointLP::start(IOService *provider) {
    this->pins = sptlp_pins;
    this->npins = ARRAY_SIZE(sptlp_pins);
    this->groups = sptlp_groups;
    this->ngroups = ARRAY_SIZE(sptlp_groups);
    this->functions = sptlp_functions;
    this->nfunctions = ARRAY_SIZE(sptlp_functions);
    this->communities = sptlp_communities;
    this->ncommunities = ARRAY_SIZE(sptlp_communities);

    IOLog("%s::Loading GPIO Data for SunrisePoint-LP\n", getName());

    return VoodooGPIO::start(provider);
}
