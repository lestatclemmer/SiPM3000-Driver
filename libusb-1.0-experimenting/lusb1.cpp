#include <cstring>
#include <iostream>
#include <iomanip>
#include <vector>

#include <libusb.h>

static const uint16_t BRIDGEPORT_VID = 0x1fa4;
static const uint8_t KERNEL_DETACH_INTERFACE = 0;
static const uint8_t DETECTOR_INTERFACE = 1;
static const int CMD_OUT_EP = 0x01;
static const int CMD_IN_EP = 0x81;
static const int DATA_OUT_EP = 0x02;
static const int DATA_IN_EP = 0x82;

void print_serial(libusb_device_handle* han)
{
    unsigned char write_buf[64], read_buf[64];
    write_buf[0] = 4;
    write_buf[2] = 64;
    int transferred;
    int ret = libusb_bulk_transfer(han, CMD_OUT_EP, write_buf, 64, &transferred, 1000);
    if (ret < 0) { std::cerr << "problem writing command: " << libusb_strerror(ret) << std::endl; }
    if (transferred != 64) { std::cerr << "transferred wrong # of bytes" << std::endl; }

    ret = libusb_bulk_transfer(han, DATA_IN_EP, read_buf, 64, &transferred, 1000);
    if (ret < 0) { std::cerr << "problem writing command: " << libusb_strerror(ret) << std::endl; }
    if (transferred != 64) { std::cerr << "transferred wrong # of bytes" << std::endl; }
    
    auto f = std::cout.flags();
    std::stringstream ss;
    for (size_t i = 8; i < 24; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << std::uppercase << (0xff & read_buf[i]);
    }
    std::cout.flags(f);

    std::cout << "Serial number: " << ss.str() << std::endl;
}

std::vector<libusb_device_handle*> claim_bp_devs(libusb_device** devs, ssize_t num_devs)
{
    auto f = std::cout.flags();
    std::vector<libusb_device_handle*> v;

    for (ssize_t i = 0; i < num_devs; ++i) {
        libusb_device* dev = devs[i];
        libusb_device_descriptor desc;
        int ret = libusb_get_device_descriptor(dev, &desc);
        std::cout << "Vendor ID: " << std::hex << +desc.idVendor << std::endl;
        std::cout << "Product ID: " << std::hex << std::nouppercase << "0x" << +desc.idProduct << std::endl;
        if (desc.idVendor == BRIDGEPORT_VID) {
            std::cout << "*** Bridgeport device!" << std::endl;
            libusb_device_handle* han = nullptr;
            ret = libusb_open(dev, &han);
            // just get it on the stack before throwing
            v.push_back(han);

            if (ret < 0) {
                throw std::runtime_error("Failed to open device. hmm...");
            }

            // detach/retach kernel driver from interfaces we claim
            ret = libusb_set_auto_detach_kernel_driver(han, 1);
            if (ret < 0) {
                std::cerr << "couldn't auto-detach kernel driver: " << libusb_strerror(ret) << std::endl;
            }

            ret = libusb_claim_interface(han, DETECTOR_INTERFACE);
            if (ret < 0) {
                std::cerr << "couldn't claim interface: " << libusb_strerror(ret) << std::endl;
            }
        }
    }

    std::cout.flags(f);
    return v;
}

int main()
{
    libusb_context* ctx;
    int ret = libusb_init(&ctx);
    if (ret < 0) {
        std::cerr << "Error initializing libusb context: " << libusb_strerror(ret) << std::endl;
        return 1;
    }
    ssize_t num_devs;
    
    // log warnings.
    ret = libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
    if (ret < 0) {
        std::cerr << "Error setting log level: " << libusb_strerror(ret) << std::endl;
        return 1;
    }

    libusb_device** devs;
    num_devs = libusb_get_device_list(ctx, &devs);
    if (num_devs < 0) {
        std::cerr << "Error getting device list: " << libusb_strerror(num_devs) << std::endl;
        return 1;
    }

    auto bridgeport_handles = claim_bp_devs(devs, num_devs); 

    // free the list & unreference the devices when we're done
    libusb_free_device_list(devs, 1);
    devs = nullptr;
    std::cout << "devs addr " << devs << std::endl;

    for (auto& han : bridgeport_handles) {
        std::cout << "Claimed BP dev " << han << std::endl;
        print_serial(han);
        ret = libusb_release_interface(han, DETECTOR_INTERFACE);
        if (ret < 0) { std::cerr << "Error releasing interface: " << libusb_strerror(ret) << std::endl; }
        libusb_close(han);
    }

    // clean up the context
    libusb_exit(ctx);
    return 0;
}
