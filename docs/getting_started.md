# What is Project FireFly?

Project FireFly is a lighting control system for the home and small business.  It aims to democratize reliable, high-quality controls at an affordable pricepoint.  There are two main components to Project FireFly: the controller and client.

Using low-cost Arduino-based microcontrollers, Project FireFly makes software defiend lighting controls available to the masses at a moderate premium cost to traditional lighting controls, and a slight premium cost to "smart lighting" controls integrated with existing switches.  Similar commercial applications, while fancier, can add 5-10% to a new project cost compared to traditional lighting controls.  On a $300,000 new house build, that's $15,000 to $30,000...just for lighting controls! 🤯

**Open-Source Hardware**
Project FireFly uses open-source hardware to build it yourself, manufacture it to spec, or design your own using ours as a reference.

**Open-Source, Locally-Controlled Software**
No for-profit company should ever decide they can "stop supporting" or "end of life" your technology in the interest of selling you something slightly newer, but with essentially the same capabilities you already have.  Sending data to _their_ cloud costs money and is unnecessary.

**Native API's**
HTTP and MQTT work right out of the box, so you can integrate lighting controls to almost any other system imaginable.  What's more is that state of health and configuration are all on-board -- no apps to install.

**We ❤️ Home Assistant**
We love Home Assistant and other open home automation systems.  FireFly was designed from the ground-up to work with them natively.  From self discovery <Badge type="warning" text="TODO" /> to daily operation via MQTT <Badge type="warning" text="TODO" />, you can make your lighting controls work 100% locally.

## FireFly Controller
FireFly Controller is the heart of the system.  It is the central hub for accepting inputs -- from a human touching a button to an automation signaling a change is necessary -- and processing outputs to the [high voltage relays](relays) that make our lives more enjoyable.

The hardware consists of a custom PCB with RJ45 inputs that connect to FireFly clients, an Ethernet controller, OLED display, front panel selection button, and outputs.  An ESP32 is the core computing module.  In a typical home, one FireFly Controller should be able used per floor; in a small home a single controller may suffice.

FireFly Controller is designed to be operational all by itself.  The "other half" will appreciate this fail-safe, because it's as dependable as what they've used everywhere else:
- ✅ You still have local control of all circuits when your home automation is offline
- ✅ You still have local control of all circuits when your LAN or WiFi is having trouble
- ✅ You still have local control of all circuits when your Internet is offline
- 🚫 May not work during a zombie apocalypse 🧟

### Application Software
FireFly Controller features two applications contained within this repository, the Hardware Registration and Configuration application, and the main Controller application.

### Hardware Registration and Configuration Application
The Hardware Registration and Configuration is for use with new, unprogrammed boards -- essentially factory use.  The application provides both a web interface and underlying API calls used by the web interface.

The application's primary functions are:
- Set the device's [partition scheme](ide_configuration#partitions)
- Set the external EEPROM with identity information
- Hardware quality assurance
- Register the hardware with the cloud service for remote backup of configuration data <Badge type="warning" text="TODO" />

### Controller Application
The main application for production use that accepts input from a physical switch and can, optionally, send PWM voltage to an output.  It also supports HTTP <Badge type="warning" text="TODO" /> and MQTT <Badge type="warning" text="TODO" /> for inputs from external sources, such as a home automation system.  Inputs sensed will raise events via MQTT for use in a home automation system.  FireFly Controller is designed to be paired with FireFly Clients.

## FireFly Client <Badge type="tip" text="Coming Soon" />
FireFly Clients are devices that usually replace one or more lightswitches.  A Client can support multiple LED-illuminated buttons for feedback when they are pressed.  They communicate directly with the FireFly controller over Category 5 or Category 6 Ethernet cable but _do not_ use IP to communicate.  Clients connect to MQTT via WiFi and subscribe to certain messages to know when to provide visual feedback of a button press.  Because a blinking LED isn't essential to use the system, they still function even during those offline moments.

