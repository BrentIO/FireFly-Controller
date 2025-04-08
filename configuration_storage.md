# About Controller and Client Configurations
Controllers store data on the configFS file system as minified JSON files, stored by the device's UUID in the `/configFS/{type}/` directory, where `{type}` is either `controllers` or `clients`. All controllers will have definitions for all other controllers and clients in their own file system for redundancy.

## Configuration File Contents
The configurations contain only override data for each entity.  Default data is not populated should not be populated in the file.  For example, an input that is normally open, enabled, and does not monitor for a long change should not be populated in the file, because all of these parameters match defaults.

There are no limitations made at the time of creation to ensure the data contained in the request is accurate, for example, that the number of ports defined matches the maxium number of physical ports.  If the number of ports defined exceeds the physical number of ports, the controller will not attempt to read them into memory and will be ignored.  However, the payload will be checked to validate it is a valid JSON document before being persisted.

Below are the defaults below which do NOT need to be specified in the payload.  Refer to the Swagger for additional information:

### Controller
| Parameter | Default Value |
| --------- | ------------- |
| (No default values) |

### Inputs
| Parameter | Default Value |
| --------- | ------------- |
| `type`      | `NORMALLY_OPEN` |
| `long_change` | `false` |
| `enabled`   | `true` |

### Outputs
| Parameter | Default Value |
| --------- | ------------- |
| `type`      | `BINARY` |
| `enabled`   | `true` |


<Badge type="tip" text="Tip" /> For client configuration defaults, see the <Badge type="warning" text="TODO" /> Client configuration page.


## Startup
As the controller starts, the following sequence will occur:

- The controller will check if a file matching the controller's UUID exists in the `/configFS/devices/` directory.  If no match is found, the process ends without configuring any inputs or outputs, and all inputs and outputs are enabled.

- Contents of the file will be read and loaded into memory.

- Contents of the `outputs` will be processed.

- Contents of the `ports` will be processed. 
    - For each port, `channels` will be processed.
        - For each channel, the `actions` will be processed.