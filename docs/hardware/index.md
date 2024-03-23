Hardware can be identified by the product ID (PID), which consists of `FFC` for Controllers and the number of inputs followed by the number of outputs.  The suffix of the PID indicates the two-digit year and two-digit month of the design release.  All single-digit values have a leading zero.  For example, a device with PID FFC0806-2305 is a FireFly Controller with 8 inputs, 6 outputs, with a final design dated May 2023.

Hardware is also designated a hex code, which is a hexidecimal representation of the numeric portion of the PID.  For example, FFC0806-2305 has a hex code of `0x08062305`.

# Hardware Versions
| PID | Hex Code | Status |
| --- | -------- | ------ |
| [FFC3232-2211](FFC3232_2211) | 0x32322211 | Internal/Unreleased |
| [FFC0806-2305](FFC0806_2305) | 0x08063205 | Internal/Unreleased |
| FFC3232-WIP | 0x3232TBD | Work in Progress |
| FFC0806-WIP | 0x0806TBD | Work in Progress |

# Adding a new hardware version
Hardware configurationsk are abstracted from the main applications to allow for compilation with minimal hardware-specific design considerations.  Each hardware model is defined in `hardware.h`.