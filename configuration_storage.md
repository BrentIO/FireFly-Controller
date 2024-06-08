# About Controller and Client Configurations
Controllers store data on the configFS file system as minified JSON files.  The file structure is roughly:
```
SEE DESKTOP FOR FILE STRUCTURE EXAMPLE
```

The json files contain the payloads that were posted to the API endpoint.  For example, calling `/api/controllers/07c69a59-26be-45ab-80f2-e4ff9e96a029` endpoint will retrieve the content of `/ctls/07c69a59-26be-45ab-80f2-e4ff9e96a029/ctrl.json`.  In the example above, controller ID 1 has a configuration for the controller plus two port definitions.  Within the port definition, the channels contain overrides for the given channel number (in this example, channels 1 and 2 for port 1 and channel 4 for port 5).  The channel overrides contain the payload sent to the `/api/controllers/{id}/input/{input}/channels/{channel}` endpoint.

## Startup
As the controller starts, the following sequence will occur:

- The controller will search each child of `/ctlrs/` until it finds the ctrl.json file which contains a device UUID matching the device UUID programmed in the EEPROM.  All controllers will have definitions for all other controllers in their own filesystem for redundancy.  If no match is found, the process ends without configuring any inputs or outputs, and no inputs or outputs are enabled.

- Contents of the `/ctrls/{id}/outputs/` will be read and loaded into memory.  The output port numbers may not be sequential.  Because only defined output ports will be updated in memory, output ports which have not been defined will not be enabled.

- Contents of the `/ctrls/{id}/inputs/` will be read and loaded into memory.  Because only defined inputs will be loaded into memory, inputs which have not been defined will not be enabled.  Likewise, if an input is defined but there are channels which are not defined, the channel will not be enabled.  Changes sensed on those input channels will be ignored.

There are no limitations made at the time of creation to ensure the number of ports defined matches the maxium number of physical ports.  If the number of ports defined exceeds the physical number of ports, the controller will not attempt to read them into memory and will be ignored.  For example, if port 33 is defined on a controller with only 32 ports, port 33 configuration will be ignored but the configuration file will continue to exist on the filesystem.

## Predefining Configurations
Configuration data can be predefined before the controller UUID is known by inserting a random device UUID into the payload.  When the device's actual UUID is known, a `PATCH` request can be made to the `/api/controllers/{id}` endpoint with the correct UUID.