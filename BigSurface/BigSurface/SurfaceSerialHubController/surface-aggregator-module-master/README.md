# Linux ACPI (Platform) Drivers for 5th Gen. Surface Devices

Linux embedded controller driver for 5th generation (and later) Surface devices required for battery status and more.

_Note: These modules are integrated into https://github.com/linux-surface/linux-surface._
_There is no need to install it separately if you've already installed one of the kernels provided there._
If you have a Surface Book 2 you might also want to have a look at the [dtx-daemon][dtx-daemon] and the [surface-control][surface-control] utility.

## Supported Features and Devices

| Device                 | Supported Features                                                           | Known Issues/Missing Features |
|------------------------|------------------------------------------------------------------------------|-------------------------------|
| Surface Book 2         | lid status, battery status, clipboard detach events, performance-modes, dGPU | -                             |
| Surface Book 3         | lid status, battery status, performance-modes, dGPU                          | -                             |
| Surface Laptop         | battery status, keyboard                                                     | -                             |
| Surface Laptop 2       | battery status, keyboard                                                     | -                             |
| Surface Laptop 3 (13") | battery status, keyboard, performance-modes                                  | -                             |
| Surface Laptop 3 (15") | battery status, keyboard, performance-modes                                  | -                             |
| Surface Laptop Go      | battery status, performance-modes (NB: keyboard supported OOTB upstream)     | -                             |
| Surface Pro 2017       | battery status, performance-modes                                            | -                             |
| Surface Pro 6          | battery status, performance-modes                                            | -                             |
| Surface Pro 7          | battery status, performance-modes                                            | -                             |

If you want to help out, have a look at the corresponding issues.
In most cases, we just need a bit of information from someone who owns such a device.
Also, if you think there's anything missing here, feel free to open an issue!

_For more details, please have a look at the [wiki][wiki]._

[wiki]: https://github.com/linux-surface/surface-aggregator-module/wiki
[dtx-daemon]: https://github.com/linux-surface/surface-dtx-daemon
[surface-control]: https://github.com/linux-surface/surface-control
