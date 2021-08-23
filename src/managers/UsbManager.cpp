#include "UsbManager.hpp"

namespace SipmUsb
{
    UsbManager::UsbManager() :
        BaseManager(),
        usb_ctx(nullptr), raw_devs(nullptr)
    {
        int ret = libusb_init(&usb_ctx);
        if (ret < 0) {
            log_err(libusb_strerror(ret));
            throw std::runtime_error("Failed to initialize USB context");
        }

        ret = libusb_set_option(usb_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
        if (ret < 0) {
            std::stringstream ss;
            ss << "Error setting log level: " << libusb_strerror(ret) << std::endl;
            log_err(ss.str());
            throw std::runtime_error(ss.str());
        }

        claim_bridgeport_devices();
        map_bridgeport_devices();

        if (raw_devs) libusb_free_device_list(raw_devs, 1);
        raw_devs = nullptr;
    }

    UsbManager::~UsbManager()
    {
        if (raw_devs) libusb_free_device_list(raw_devs, 1);
        raw_devs = nullptr;
        int ret = 0;
        for (auto& han_wrap : devices) {
            auto han = han_wrap.new_han;
            if (han) {
                ret = libusb_release_interface(han, DETECTOR_INTERFACE);
                if (ret != 0) { log_err("Failed to release claimed device."); }
                libusb_close(han);
                if (ret != 0) { log_err("Failed to close claimed device."); }
            }
        }
        if (usb_ctx) libusb_exit(usb_ctx);
    }

    void UsbManager::claim_bridgeport_devices()
    {
        ssize_t num_devs = libusb_get_device_list(usb_ctx, &raw_devs);
        if (num_devs < 0) {
            std::stringstream ss;
            ss << "Error getting device list: " << libusb_strerror(num_devs) << std::endl;
            log_err(ss.str());
            throw std::runtime_error(ss.str());
        }
        
        for (ssize_t i = 0; i < num_devs; ++i) {
            libusb_device* dev = raw_devs[i];
            libusb_device_descriptor desc;

            int ret = libusb_get_device_descriptor(dev, &desc);
            if (ret < 0) {
                std::stringstream ss;
                ss << "Error getting device descriptor: " << libusb_strerror(num_devs);
                log_err(ss.str());
                throw std::runtime_error(ss.str());
            }
            
            if (desc.idVendor == UsbManager::BRIDGEPORT_VID) {
                LibUsbDeviceWrap dw;
                dw.new_dev = dev;
                claim_single_bp_dev(dw);
            }
        }
    }

    void UsbManager::claim_single_bp_dev(LibUsbDeviceWrap& dev_wrap)
    {
        libusb_device* dev = dev_wrap.new_dev;
        libusb_device_handle* han = nullptr;
        int ret = libusb_open(dev, &han);
        // just get it into the vector before throwing anything
        LibUsbHandleWrap hw;
        hw.new_han = han;
        devices.push_back(hw);

        if (ret < 0) {
            std::stringstream ss;
            ss << "Failed to open BP device: " << libusb_strerror(ret);
            throw std::runtime_error(ss.str());
        }

        // detach/retach kernel driver from interfaces we claim
        ret = libusb_set_auto_detach_kernel_driver(han, 1);
        if (ret < 0) {
            std::stringstream ss;
            ss << "couldn't auto-detach kernel driver: " << libusb_strerror(ret);
            log_err(ss.str());
            throw std::runtime_error(ss.str());
        }

        ret = libusb_claim_interface(han, DETECTOR_INTERFACE);
        if (ret < 0) {
            std::stringstream ss;
            ss << "couldn't claim interface: " << libusb_strerror(ret);
            log_err(ss.str());
            throw std::runtime_error(ss.str());
        }
    }

    int UsbManager::xfer_in_chunks(LibUsbHandleWrap& han_wrap, int endpoint, void* raw_buffer, int num_bytes, int timeout)
    {
        // wow
        unsigned char* buffer = (unsigned char*) raw_buffer; 
        libusb_device_handle* han = han_wrap.new_han;

        // ARM processor inside detector only has 256-byte buffer
        static const int CHUNK_SZ = 256;
        int nchunks = num_bytes / CHUNK_SZ;
        int leftover = num_bytes % CHUNK_SZ;
        int ret = 0;
        int transferred = 0;

        // transfer the 256 byte chunks
        for (int i = 0; i < nchunks; ++i) {
            ret = libusb_bulk_transfer(han, endpoint, buffer + i*CHUNK_SZ, CHUNK_SZ, &transferred, timeout);
            if (transferred != CHUNK_SZ) {
                std::stringstream ss;
                ss << "didn't read/write appropriate chunk size: " << transferred << " vs " << CHUNK_SZ << ". " << libusb_strerror(ret);
                log_err(ss.str());
            }
            if (ret < 0) return ret;
        }

        transferred = 0;
        // transfer the rest
        if (leftover != 0) {
            ret = libusb_bulk_transfer(han, endpoint, buffer + nchunks*CHUNK_SZ, leftover, &transferred, timeout);
            if (transferred != leftover) {
                std::stringstream ss;
                ss << "didn't read/write appropriate leftover: " << transferred << " vs " << leftover << ". " << libusb_strerror(ret);
                log_err(ss.str());
            }
            if (ret < 0) return ret;
        }
        return 0;
    }
}
