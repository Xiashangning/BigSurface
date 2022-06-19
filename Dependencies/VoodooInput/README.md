VoodooInput
===========

[![Build Status](https://github.com/acidanthera/VoodooInput/workflows/CI/badge.svg?branch=master)](https://github.com/acidanthera/VoodooInput/actions) [![Scan Status](https://scan.coverity.com/projects/22196/badge.svg?flat=1)](https://scan.coverity.com/projects/22196)

An opensource trackpad aggregator kernel extension providing Magic Trackpad 2 software emulation for arbitrary input sources like [VoodooPS2](https://github.com/acidanthera/VoodooPS2).

**WARNING**: This kernel extension is designed to be bundled with a dedicated multitouch client driver and exists for the developers only.
Please download the latest version of [VoodooPS2](https://github.com/acidanthera/VoodooPS2/releases) or [VoodooI2C](https://github.com/VoodooI2C/VoodooI2C/releases)
to make use of this kext.

#### Credits
- [Apple](https://www.apple.com) for macOS
- [VoodooI2C](https://github.com/alexandred/VoodooI2C) [Team](https://github.com/alexandred/VoodooI2C/graphs/contributors) ([alexandred](https://github.com/alexandred), [ben9923](https://github.com/ben9923), [blankmac](https://github.com/blankmac), [coolstar](https://github.com/coolstar), and others) for Magic Trackpad 2 reverse engineering, implementation, and reference example
- [kprinssu](https://github.com/kprinssu) for writing the majority of this project and initial trackpad integration
- [usr-sse2](https://github.com/usr-sse2) for further additions and force touch support
- [vit9696](https://github.com/vit9696) for minor compatibility fixes
