Hardware can be identified by the product ID (PID), which is made up of `FFC` and the number of inputs followed by the number of outputs.  The suffix of the PID indicates the year and month of the design release.  All single-digit values have a leading zero.  For example, FFC0806-2305 is a FireFly Controller with 8 inputs, 6 outputs, with a final design dated May 2023.

# Hardware Abstraction Layer

Hardware configurations are abstracted from the main application to allow for compilation without any hardware-specific design considerations.  Each hardware model is defined in `hardware.h`.


# Hardware Versions
| PID | Hex Code | Status |
| --- | -------- | ------ |
| [FFC3232-2211](ffc3232-2211) | 0x32322211 | Internal/Unreleased |
| [FFC0806-2305](FFC0806-2305) | 0x08063205 | Internal/Unreleased |
| [FFC3232-WIP](FFC3232-2211) | 0x3232TBD | Work in Progress |
| [FFC0806-WIP](FFC0806-2305) | 0x0806TBD | Work in Progress |
