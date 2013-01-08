.PHONY: all build clean upload size upload_using_programmer burn_bootloader

all: build

build: clean build_msg ./build/ISS_LAMP_ARDUINO.hex size

build_msg:
	@echo "Building..."

upload: build
	@echo "Uploading ./build/ISS_LAMP_ARDUINO.hex..."
	@"avrdude" "-CC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/tools/avr/etc/avrdude.conf" -q -q -V -patmega328p -carduino -PCOM15 -b115200 -D "-Uflash:w:./build/ISS_LAMP_ARDUINO.hex:i"

upload_using_programmer: build
	@echo "Uploading ./build/ISS_LAMP_ARDUINO.hex Using AVR ISP..."
	@"avrdude" "-CC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/tools/avr/etc/avrdude.conf" -q -q -V -patmega328p -cstk500v1  "-Uflash:w:./build/ISS_LAMP_ARDUINO.hex:i"

burn_bootloader:
	@echo "Burning Bootloader..."
	@"avrdude" "-CC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/tools/avr/etc/avrdude.conf" -q -q -V -patmega328p -cstk500v1  -e -Ulock:w:0x3F:m -Uefuse:w:0x05:m -Uhfuse:w:0xde:m -Ulfuse:w:0xff:m
	@"avrdude" "-CC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/tools/avr/etc/avrdude.conf" -q -q -V -patmega328p -cstk500v1  "-Uflash:w:C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/avr/bootloaders/optiboot_atmega328.hex:i" -Ulock:w:0x0F:m

size: ./build/ISS_LAMP_ARDUINO.hex
	@echo "Size Information Of ./build/ISS_LAMP_ARDUINO.hex:"
	@"avr-size" -A "./build/ISS_LAMP_ARDUINO.hex"
	@echo Maximum Size: 32256 bytes.

clean:
	@echo "Cleaning..."
	@"rm" -rf "./build/ISS_LAMP_ARDUINO.hex" "./build/ISS_LAMP_ARDUINO.elf" "./build/ISS_LAMP_ARDUINO.eep" "./build/ISS_LAMP_ARDUINO.cpp.o" "./build/core.a" "./build/CDC.cpp.o" "./build/HardwareSerial.cpp.o" "./build/HID.cpp.o" "./build/IPAddress.cpp.o" "./build/main.cpp.o" "./build/new.cpp.o" "./build/Print.cpp.o" "./build/Stream.cpp.o" "./build/Tone.cpp.o" "./build/USBCore.cpp.o" "./build/WInterrupts.c.o" "./build/wiring.c.o" "./build/wiring_analog.c.o" "./build/wiring_digital.c.o" "./build/wiring_pulse.c.o" "./build/wiring_shift.c.o" "./build/WMath.cpp.o" "./build/WString.cpp.o" "./build/SPI.cpp.o" "./build/Dhcp.cpp.o" "./build/Dns.cpp.o" "./build/Ethernet.cpp.o" "./build/EthernetClient.cpp.o" "./build/EthernetServer.cpp.o" "./build/EthernetUdp.cpp.o" "./build/socket.cpp.o" "./build/w5100.cpp.o" "./build/EthernetDNS.cpp.o" "./build/EthernetCompat.cpp.o" "./build/EthernetUtil.c.o"

./build/ISS_LAMP_ARDUINO.hex: ./build/ISS_LAMP_ARDUINO.elf ./build/ISS_LAMP_ARDUINO.eep
	@echo "Creating ./build/ISS_LAMP_ARDUINO.hex..."
	@"avr-objcopy" -O ihex -R .eeprom "./build/ISS_LAMP_ARDUINO.elf" "./build/ISS_LAMP_ARDUINO.hex"

./build/ISS_LAMP_ARDUINO.eep: ./build/ISS_LAMP_ARDUINO.elf
	@echo "Creating ./build/ISS_LAMP_ARDUINO.eep..."
	@"avr-objcopy" -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 "./build/ISS_LAMP_ARDUINO.elf" "./build/ISS_LAMP_ARDUINO.eep"

./build/ISS_LAMP_ARDUINO.elf: ./build/core.a ./build/ISS_LAMP_ARDUINO.cpp.o
	@echo "Creating ./build/ISS_LAMP_ARDUINO.elf..."
	@"avr-gcc" -Os -Wl,--gc-sections -mmcu=atmega328p -o "./build/ISS_LAMP_ARDUINO.elf"  "./build/ISS_LAMP_ARDUINO.cpp.o" "./build/core.a" "-L./build" -lm

./build/core.a: ./build/CDC.cpp.o ./build/HardwareSerial.cpp.o ./build/HID.cpp.o ./build/IPAddress.cpp.o ./build/main.cpp.o ./build/new.cpp.o ./build/Print.cpp.o ./build/Stream.cpp.o ./build/Tone.cpp.o ./build/USBCore.cpp.o ./build/WInterrupts.c.o ./build/wiring.c.o ./build/wiring_analog.c.o ./build/wiring_digital.c.o ./build/wiring_pulse.c.o ./build/wiring_shift.c.o ./build/WMath.cpp.o ./build/WString.cpp.o ./build/SPI.cpp.o ./build/Dhcp.cpp.o ./build/Dns.cpp.o ./build/Ethernet.cpp.o ./build/EthernetClient.cpp.o ./build/EthernetServer.cpp.o ./build/EthernetUdp.cpp.o ./build/socket.cpp.o ./build/w5100.cpp.o ./build/EthernetDNS.cpp.o ./build/EthernetCompat.cpp.o ./build/EthernetUtil.c.o
	@echo "Creating ./build/core.a..."
	@"avr-ar" rcs "./build/core.a"  "./build/CDC.cpp.o" "./build/HardwareSerial.cpp.o" "./build/HID.cpp.o" "./build/IPAddress.cpp.o" "./build/main.cpp.o" "./build/new.cpp.o" "./build/Print.cpp.o" "./build/Stream.cpp.o" "./build/Tone.cpp.o" "./build/USBCore.cpp.o" "./build/WInterrupts.c.o" "./build/wiring.c.o" "./build/wiring_analog.c.o" "./build/wiring_digital.c.o" "./build/wiring_pulse.c.o" "./build/wiring_shift.c.o" "./build/WMath.cpp.o" "./build/WString.cpp.o" "./build/SPI.cpp.o" "./build/Dhcp.cpp.o" "./build/Dns.cpp.o" "./build/Ethernet.cpp.o" "./build/EthernetClient.cpp.o" "./build/EthernetServer.cpp.o" "./build/EthernetUdp.cpp.o" "./build/socket.cpp.o" "./build/w5100.cpp.o" "./build/EthernetDNS.cpp.o" "./build/EthernetCompat.cpp.o" "./build/EthernetUtil.c.o"

./build/ISS_LAMP_ARDUINO.cpp.o: ./build/ISS_LAMP_ARDUINO.cpp
	@echo "Creating ./build/ISS_LAMP_ARDUINO.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/CDC.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/CDC.cpp
	@echo "Creating ./build/CDC.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/HardwareSerial.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/HardwareSerial.cpp
	@echo "Creating ./build/HardwareSerial.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/HID.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/HID.cpp
	@echo "Creating ./build/HID.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/IPAddress.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/IPAddress.cpp
	@echo "Creating ./build/IPAddress.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/main.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/main.cpp
	@echo "Creating ./build/main.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/new.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/new.cpp
	@echo "Creating ./build/new.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Print.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/Print.cpp
	@echo "Creating ./build/Print.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Stream.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/Stream.cpp
	@echo "Creating ./build/Stream.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Tone.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/Tone.cpp
	@echo "Creating ./build/Tone.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/USBCore.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/USBCore.cpp
	@echo "Creating ./build/USBCore.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/WInterrupts.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/WInterrupts.c
	@echo "Creating ./build/WInterrupts.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/wiring.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/wiring.c
	@echo "Creating ./build/wiring.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/wiring_analog.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/wiring_analog.c
	@echo "Creating ./build/wiring_analog.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/wiring_digital.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/wiring_digital.c
	@echo "Creating ./build/wiring_digital.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/wiring_pulse.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/wiring_pulse.c
	@echo "Creating ./build/wiring_pulse.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/wiring_shift.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/wiring_shift.c
	@echo "Creating ./build/wiring_shift.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/WMath.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/WMath.cpp
	@echo "Creating ./build/WMath.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/WString.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino/WString.cpp
	@echo "Creating ./build/WString.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/SPI.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI/SPI.cpp
	@echo "Creating ./build/SPI.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Dhcp.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/Dhcp.cpp
	@echo "Creating ./build/Dhcp.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Dns.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/Dns.cpp
	@echo "Creating ./build/Dns.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/Ethernet.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/Ethernet.cpp
	@echo "Creating ./build/Ethernet.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetClient.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/EthernetClient.cpp
	@echo "Creating ./build/EthernetClient.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetServer.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/EthernetServer.cpp
	@echo "Creating ./build/EthernetServer.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetUdp.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/EthernetUdp.cpp
	@echo "Creating ./build/EthernetUdp.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/socket.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility/socket.cpp
	@echo "Creating ./build/socket.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/w5100.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility/w5100.cpp
	@echo "Creating ./build/w5100.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetDNS.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/EthernetDNS.cpp
	@echo "Creating ./build/EthernetDNS.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetCompat.cpp.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility/EthernetCompat.cpp
	@echo "Creating ./build/EthernetCompat.cpp.o..."
	@"avr-g++" -c -g -Os -w -fno-exceptions -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

./build/EthernetUtil.c.o: C:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility/EthernetUtil.c
	@echo "Creating ./build/EthernetUtil.c.o..."
	@"avr-gcc" -c -g -Os -w -ffunction-sections -fdata-sections -MMD -mmcu=atmega328p -DF_CPU=16000000L -DARDUINO=1.0.2  "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/cores/arduino" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/hardware/arduino/variants/standard" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/SPI" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS" "-I./build" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/Ethernet/utility" "-IC:/Users/Mark/Desktop/Hack/arduino-1.0.2/libraries/EthernetDNS/utility" "-I."  "$<" -o "$@"

