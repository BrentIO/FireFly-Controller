FireFly controller supports both binary (on/off) and proportional (dimming) to be configured on every output port.

::: warning Check with local ordinances
Check with local ordinances about any requirements related to switching AC voltage and any requirements or permits that might be necessary.
:::

::: info Always consult with a licensed electrician
Handling high voltage can cause injury or death.  Damage to property can occur if not installed and handled according to local laws and ordinances.  Always consult with a licensed electrican.
:::

# Known Solid State Relays
Solid state relays use light, rather than mechanical switches, to open and close the high voltage side of the relay using 0-5VDC proportional control.  For binary relays, they will turn on at the minimum voltage and turn off below a minimum voltage when applying 5VDC.

## Binary Controls
Binary controls come in two forms: traditional solid state relays and contactor relays.  Check with your local laws if one must be used over another.  However, a good rule of thumb is that if you are connecting a typical light circuit as the load, a standard relay is typically sufficient.  If connecting a motor, a contactor relay should be used.

## Relay
[Crydom DR22 Series DIN Rail Mount SSRs](https://www.sensata.com/products/relays/dr22-series-din-rail-mount-ac-output-ssr-dr2260d20u) are reliable and can typically handle more load than found in a normal household lighting setup.  Refer to the datasheet to ensure you are meeting your specific electrical needs, but `DR2220D20U` is a good choice in North America and is C-UL-US approved.  These devices typically ship with a pre-installed heatsink.

Wiring on this device can be confusing; be sure to connect the correct voltage to the correct terminal.  Refer to the data sheet and [manufacturer's installation guide](https://www.sensata.com/sites/default/files/a/sensata-dr22-series-din-rail-mount-ssr-installation.pdf) for details.

5VDC from the FireFly Controller should be connected ot A1 (+) and A2 (-).  Only two wires are needed from the controller for this relay: `Red` (+) and `Black` (-).

## Contactor
[Crydom DR22 Series DIN Rail Mount SSRs](https://www.sensata.com/products/relays/dr22-series-din-rail-mount-ac-output-ssr-dr2260d20v) are reliable and can be used to power bathroom exhaust fans, small pumps, and other needs where a motor is (or could be) connected.  Refer to the datasheet to ensure you are meeting your specific electrical needs, but `DR2260D20V` is a good choice in North America and is C-UL-US approved.  These devices typically ship with a pre-installed heatsink.

Refer to the data sheet and [manufacturer's installation guide](https://www.sensata.com/sites/default/files/a/sensata-dr22-series-din-rail-mount-ssr-installation.pdf) for details.

5VDC from the FireFly Controller should be connected ot A1 (+) and A2 (-).  Only two wires are needed from the controller for this relay: `Red` (+) and `Black` (-).

## Proportional Control
Proportional relays allow a percentage of brightness from 0-100%.  In reality, proportional controls will usually be most noticeable between 10% and 90%.


::: danger Not all lights can be dimmed
Check with the manufacturer of the fixture to ensure it can be dimmed.  Many LED fixtures **cannot** be dimmed, or if they can be, require a particular type of dimmer.  If your fixture cannot be dimmed, or if you do not use the correct type of dimmer, **damage or fire may occur**.  

Crydom PMP2425 proportional relays use a triac dimmer.  If you are not certain the dimmer type supported by the fixture **do not attempt to dim it**.
:::

[Crydom PMP2425W](https://www.sensata.com/products/relays/pmp-series-proportional-control-ssr-pmp2425w) is a good choice for dimming.  It uses 3 connectors to provide dimming on a 0-5VDC basis which is mapped from 0% to 100%.  This model typically _does not_ ship with a heatsink, which is required.  The [HS301DR](https://www.sensata.com/sites/default/files/a/sensata-hs301dr-heatsink-ssr-assemblies-datasheet.pdf) has been known to work well, but determine your own thermal needs based on installation location and cooling available.

When using this dimmer, ensure the switch is set to `A`.  Wires at both the relay and the controller should be `Red` (V), `Green` (+), `Black` (-).

Refer to the data sheet and [manufacturer's installation guide](https://www.sensata.com/sites/default/files/a/sensata-pmp%20series-installation-sheet.pdf) for details.

**Note:** Some fixtures, even though they support dimming, may display slight flickering or not turn on at certain percentages, particularly less than 50% brightness.