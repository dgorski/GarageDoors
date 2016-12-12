# GarageDoors
Arduino / ES8266 wifi garage door opener

Basic parts list

- ESP8266 dev kit 1.0 (NodeMCU Dev Kit 1.0)
-- https://www.amazon.com/HiLetgo-Version-NodeMCU-Internet-Development/dp/B010O1G1ES

- Relay module
-- https://www.amazon.com/Elegoo-Channel-Optocoupler-Arduino-Raspberry/dp/B01HEQF5HU

- Jumpers (connect ESP module to Relay module)
-- https://www.amazon.com/Elegoo-120pcs-Multicolored-Breadboard-arduino/dp/B01EV70C78

- micro USB power (cell phone charger)
-- Any micro-usb charger - I've used USB A/C adaptors from a Kindle, an iPhone, and a Galaxy - just have to use a micro usb cable

== Libraries

This project uses the ArduinoJson library, available in the Arduino IDE Library manager.

https://github.com/bblanchon/ArduinoJson

It also uses the ESP8266 board configs.  These are added to the IDE using the boards manager.  The boards URL for the ESP8266 is here:

https://github.com/esp8266/Arduino/blob/master/doc/installing.md

