//
//  VoodooGPIOCannonLakeLP.cpp
//  VoodooGPIO
//
//  Created by Alexandre Daoud on 15/11/18.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooGPIOCannonLakeLP.hpp"

OSDefineMetaClassAndStructors(VoodooGPIOCannonLakeLP, VoodooGPIO);

bool VoodooGPIOCannonLakeLP::start(IOService *provider) {
    this->pins = cnllp_pins;
    this->npins = ARRAY_SIZE(cnllp_pins);
    this->groups = cnllp_groups;
    this->ngroups = ARRAY_SIZE(cnllp_groups);
    this->functions = cnllp_functions;
    this->nfunctions = ARRAY_SIZE(cnllp_functions);
    this->communities = cnllp_communities;
    this->ncommunities = ARRAY_SIZE(cnllp_communities);

    IOLog("%s::Loading GPIO Data for CannonLake-LP\n", getName());

    return VoodooGPIO::start(provider);
}
