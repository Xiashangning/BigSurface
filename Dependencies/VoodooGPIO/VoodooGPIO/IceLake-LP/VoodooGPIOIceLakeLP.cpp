//
//  VoodooGPIOIceLakeLP.cpp
//  VoodooGPIO
//
//  Created by linyuzheng on 2020/8/11.
//  Copyright © 2020 CoolStar. All rights reserved.
//

#include "VoodooGPIOIceLakeLP.hpp"

OSDefineMetaClassAndStructors(VoodooGPIOIceLakeLP, VoodooGPIO);

bool VoodooGPIOIceLakeLP::start(IOService *provider) {
    this->pins = icllp_pins;
    this->npins = ARRAY_SIZE(icllp_pins);
    this->groups = icllp_groups;
    this->ngroups = ARRAY_SIZE(icllp_groups);
    this->functions = icllp_functions;
    this->nfunctions = ARRAY_SIZE(icllp_functions);
    this->communities = icllp_communities;
    this->ncommunities = ARRAY_SIZE(icllp_communities);

    IOLog("%s::Loading GPIO Data for IceLake-LP\n", getName());

    return VoodooGPIO::start(provider);
}
