#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_BIT(var,pos) ( (((var) & (pos)) > 0 ) ? (1) : (0) )

int main(int argc, char** argv)
{
    if (argc < 5) {
        fprintf(stderr, "Usage: ./libusb_hubctrl [busnumber] [idvendor] [idproduct] [portnum] [on|off]\n");
        fprintf(stderr, "Where");
        fprintf(stderr, "   busnumber: bus number where the device to be operated is, as a decimal int. See this from the output of lsusb\n");
        fprintf(stderr, "   idvendor: vendor id for the USB device to be operated, as a hexadecimal int\n");
        fprintf(stderr, "   idproduct: product id for the USB device to be operated, as a hexadecimal int\n");
        fprintf(stderr, "   portnum: The number of the port to be toggled, as decimal int\n");
        fprintf(stderr, "   on|off|status: if on, power to that port is enabled. If off, the power is disabled. If status, the status is instead read.\n");
        return 0;
    }

    const char* busnum_str = argv[1];
    const char* idvendor_str = argv[2];
    const char* idproduct_str = argv[3];
    const char* portnum_str = argv[4];
    const char* op_str = argv[5];

    int target_bus = strtol(busnum_str, 0, 10);
    int target_vendor = strtol(idvendor_str, 0, 16);
    int target_product = strtol(idproduct_str, 0, 16);
    int target_port = strtol(portnum_str, 0, 10);

    fprintf(stderr, "target bus: %d, vendor: %d, product: %d, port: %d\n",
        target_bus, target_vendor, target_product, target_port);

    const char *hexstring = "abcdef0";
    int number = (int)strtol(hexstring, NULL, 16); 

    libusb_device** devs;
    libusb_context* ctx;
    int ret;

    ret = libusb_init(&ctx);
    if (ret != 0) {
        fprintf(stderr, "Failed initting libusb\n");
        return 1;
    }

    libusb_set_debug(ctx, 3);

    int devc = libusb_get_device_list(ctx, &devs);
    if (devc == 0) {
        fprintf(stderr, "Failed getting device list\n");
        return 1;
    }

    fprintf(stderr, "There are %d devices\n", devc);

    int c;
    for (c = 0; c < devc; c++) {
        libusb_device* dev = devs[c];
        struct libusb_device_descriptor desc;
        ret = libusb_get_device_descriptor(dev, &desc);
        if (ret != 0) {
            fprintf(stderr, "Failed getting device descriptor for device %d\n", c);
            continue;
        }

        if (desc.bDeviceClass != LIBUSB_CLASS_HUB)
            continue;

        if (desc.idVendor != target_vendor && desc.idProduct != target_product)
            continue;

        uint8_t busnum = libusb_get_bus_number(dev);
        uint8_t portnum = libusb_get_port_number(dev);

        if (busnum != target_bus)
            continue;

        fprintf(stderr, "Device %d vendor: %d, product: %d, bus num %d, port num: %d\n", c, desc.idVendor, desc.idProduct,
            busnum, portnum);

        libusb_device_handle* devh;
        ret = libusb_open(dev, &devh);
        if (ret != 0) {
            fprintf(stderr, "Failed opening device: %d\n", ret);
            continue;
        }

        if (strcmp(op_str, "status") == 0) {
            int16_t buffer[4];
            memset(buffer, 0, sizeof(buffer));
            ret = libusb_control_transfer(
                devh,
                0xA3, // GET_PORT_STATUS, bmRequestType
                0x00, // GET_STATUS bRequest
                0, // wValue
                target_port, // wIndex
                (char*)buffer,
                4,
                2000
            );

            if (ret != 4) {
                fprintf(stderr, "Did not write all bytes: %d\n", ret);
                libusb_close(devh);
                continue;
            }

            // table 10-13, port status field, wPortStatus, USB 3.2 spec
            int16_t wPortStatus = buffer[0];
            int16_t wPortChange = buffer[1];
            fprintf(stderr, "wPortStatus: %d\n", wPortStatus);

            if (CHECK_BIT(wPortStatus, 0) == 0) {
                fprintf(stderr, "No device is present on the port\n");
            } else {
                fprintf(stderr, "A device is present on the port\n");
            }

            if (CHECK_BIT(wPortStatus, 1) == 0) {
                fprintf(stderr, "Port is disabled\n");
            } else {
                fprintf(stderr, "Port is enabled\n");
            }

            if (CHECK_BIT(wPortStatus, 3) == 0) {
                fprintf(stderr, "No over current condition on the port\n");
            } else {
                fprintf(stderr, "Port reports over current condition\n");
            }

            if (CHECK_BIT(wPortStatus, 4) == 0) {
                fprintf(stderr, "Port reset signal not asserted\n");
            } else {
                fprintf(stderr, "Port reset signal asserted\n");
            }

            // bits 5-8 are the link state
            uint16_t link_state = (wPortStatus >> 4) & 0x0F;
            fprintf(stderr, "link state: %x\n", link_state);

            if (CHECK_BIT(wPortStatus, 9) == 0) {
                fprintf(stderr, "Port is in the powered off state\n");
            } else {
                fprintf(stderr, "Port is not in the powered off state\n");
            }
        } else if (strcmp(op_str, "on") == 0) {
            ret = libusb_control_transfer(
                devh,
                0x23, // SET_PORT_FEATURE, bmRequestType
                0x03, // GET_STATUS bRequest
                8, // PORT_POWER, wValue
                target_port, // wIndex
                NULL, // wData
                0,  // wLength
                2000
            );

            fprintf(stderr, "set port feature ret: %d\n", ret); 
        } else if (strcmp(op_str, "off") == 0) {
            ret = libusb_control_transfer(
                devh,
                0x23, // ClearPortFeature, bmRequestType
                0x01, // GET_STATUS bRequest
                8, // PORT_POWER, wValue
                target_port, // wIndex
                NULL, // wData
                0,  // wLength
                2000
            );

            fprintf(stderr, "set port feature ret: %d\n", ret); 
        }

        libusb_close(devh);
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);

    return 0;
}

