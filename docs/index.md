---
# https://vitepress.dev/reference/default-theme-home-page
layout: home

hero:
  name: "FireFly Controller"
  text: "Software-defined lighting"
  tagline: Bringing reliable, high-quality lighting controls to the masses
  actions:
    - theme: brand
      text: FireFly Controller
      link: https://github.com/BrentIO/FireFly-Controller/tree/main/Controller
    - theme: alt
      text: Hardware Registration and Configuration
      link: https://github.com/BrentIO/FireFly-Controller/tree/main/Hardware-Registration-and-Configuration
    - theme: alt
      text: Development Environment
      link: /ide_configuration


features:
  - title: Open-Source Hardware
    details: Reference designs available for multiple mounting options with 3D printable case designs
  - title: Open-Source, Locally-Controlled Software
    details: You can review and modify our source code, so nobody can decide it's time to "sunset" your lighting controls
  - title: Native API's
    details: Integrate to other applications through HTTP and MQTT
  - title: Pre-Integrated with Home Assistant
    details: Running Home Assistant for your home automation?  Great, so are we
---


-----
# About

FireFly Controller is an Arduino-based low-voltage lighting controller.  It features two main applications contained within this repository, the Hardware Registration and Configuration application, and the main Controller application.  High-quality software-defined lighting for the home and small business is the way of the future, and this project aims to democratize reliable, high-quality controls at an affordable pricepoint.  Similar commercial applications, while fancier, can add 5-10% to a new project cost compared to traditional lighting controls.


# Hardware

The hardware consists of a custom PCB with RJ45 inputs for FireFly clients, Ethernet controller, OLED display, front panel selection button, and outputs.  An ESP32 is the core computing module.

Additional information about the hardware can be found on the [hardware page](hardware).

## Reference Designs
TODO:
- [ ] Upload final GERBER files
- [ ] Upload final schematics


# Hardware Registration and Configuration
The Hardware Registration and Configuration is for use with new, unprogrammed boards -- essentially factory use.  The application provides both a web interface and underlying API calls used by the web interface.

The application's primary functions are:
- Set the external EEPROM with identity information
- Hardware quality assurance
- Register the hardware with the cloud service for remote backup of configuration data (FUTURE)


# FireFly Controller
The main application for production use that accepts input from a physical switch and can, optionally, send PWM voltage to an output.  It also supports HTTP and MQTT for inputs from external sources, such as a home automation system.  Inputs sensed will raise events via MQTT for use in a home automation system.  FireFly Controller is designed to be paired with FireFly Clients.

TODO:
- [ ] Support HTTP
- [ ] Support MQTT
- [ ] Release FireFly Client