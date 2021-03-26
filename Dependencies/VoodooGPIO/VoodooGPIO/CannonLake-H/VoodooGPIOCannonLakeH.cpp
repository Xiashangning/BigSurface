//
//  VoodooGPIOCannonLakeH.cpp
//  VoodooGPIO
//
//  Created by Alexandre Daoud on 15/11/18.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooGPIOCannonLakeH.hpp"

OSDefineMetaClassAndStructors(VoodooGPIOCannonLakeH, VoodooGPIO);

bool VoodooGPIOCannonLakeH::start(IOService *provider) {
    this->pins = cnlh_pins;
    this->npins = ARRAY_SIZE(cnlh_pins);
    this->groups = cnlh_groups;
    this->ngroups = ARRAY_SIZE(cnlh_groups);
    this->functions = cnlh_functions;
    this->nfunctions = ARRAY_SIZE(cnlh_functions);
    this->communities = cnlh_communities;
    this->ncommunities = ARRAY_SIZE(cnlh_communities);

    IOLog("%s::Loading GPIO Data for CannonLake-H\n", getName());

    return VoodooGPIO::start(provider);
}
