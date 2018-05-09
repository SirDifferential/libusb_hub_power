# libusb_hub_power

Used to toggle power on a USB hub. See section 10.16.1 of the USB 3.2 spec for standard USB hub requests.

Requires libusb-1.0.0-dev

compile: `gcc libusb_hubctrl.c -lusb-1.0 -o libusb_hubctrl`

Get status: `sudo ./libusb_hubctrl 1 05e3 0608 1 status` 
Enable port power: `sudo ./libusb_hubctrl 1 05e3 0608 1 on` 
Disable port power: `sudo ./libusb_hubctrl 1 05e3 0608 1 off` 

