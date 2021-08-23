#include "SimManager.hpp"

namespace SipmUsb
{
    SimManager::SimManager(const std::string& lib_path) :
        BaseManager(),
        raw_funcs(lib_path)
    {
        // these three initialization functions are required for libusb to work properly
        raw_funcs.init();
        raw_funcs.find_busses();
        raw_funcs.find_devices();

        claim_bridgeport_devices();
        map_bridgeport_devices();
        // updates the counters and stuff... really weird that we need to do this.
        ping_devices();
    }

    SimManager::~SimManager()
    {
        int ret;
        for (auto& dev_han : devices) {
            auto dev = dev_han.legacy_han;
            if (dev) {
                ret = raw_funcs.release_interface(dev, DETECTOR_INTERFACE);
                if (ret != 0) { log_err("Failed to release claimed device."); }
                ret = raw_funcs.close(dev);
                if (ret != 0) { log_err("Failed to close claimed device."); }
            }
        }
    }

    void SimManager::claim_bridgeport_devices()
    {
        struct usb_bus* cur_bus = raw_funcs.get_busses();
        // step through linked list of busses
        while (cur_bus) {
            struct usb_device* cur_dev = cur_bus->devices;
            LibUsbDeviceWrap dw;
            dw.legacy_dev = cur_dev;
            // populates devices vector with usb_dev_handle*
            claim_single_bp_dev(dw);
            cur_bus = cur_bus->next;
        }
    }

    // only needed for the simulator. super weird.
    void SimManager::ping_devices()
    {
        // magic numbers from Bridgeport
        char ping_buf[64] = {0, [1]=8, [2]=60};
        for (auto& hw : devices) {
            for (int i = 0; i < 1000; ++i) {
                (void) xfer_in_chunks(hw, BaseManager::CMD_OUT_EP, ping_buf, 64, BaseManager::TIMEOUT_MS);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void SimManager::claim_single_bp_dev(LibUsbDeviceWrap& dev_wrap)
    {
        struct usb_device* dev = dev_wrap.legacy_dev;
        // step through linked list of devices on bus
        while (dev) {
            usb_device_descriptor desc = dev->descriptor;
            if (desc.idVendor == BaseManager::BRIDGEPORT_VID) {
                struct usb_dev_handle* han = raw_funcs.open(dev);
                // included in Bridgeport code but it always fails. hmm...
                (void) raw_funcs.detatch_kernel_driver_np(han, 0);  
                int ret = raw_funcs.claim_interface(han, DETECTOR_INTERFACE);
                if (ret != 0)  {
                    std::stringstream ss;
                    ss << "Couldn't claim interface: " << strerror(errno);
                    throw ClaimInterfaceException(ss.str().c_str());
                }
                LibUsbHandleWrap hw;
                hw.legacy_han = han;
                devices.push_back(hw);
            }
            dev = dev->next;
        }
    }

    int SimManager::xfer_in_chunks(LibUsbHandleWrap& han_wrap, int endpoint, void* raw_buffer, int num_bytes, int timeout)
    {
        // don't look lol
        char* buffer = (char*) raw_buffer;
        struct usb_dev_handle* han = han_wrap.legacy_han;

        // ARM processor inside detector only has 256-byte buffer
        static const int CHUNK_SZ = 256;
        int nchunks = num_bytes / CHUNK_SZ;
        int leftover = num_bytes % CHUNK_SZ;
        int ret = 0;

        // transfer the 256 byte chunks
        for (int i = 0; i < nchunks; ++i) {
            ret = raw_funcs.bulk_transfer(han, endpoint, buffer + i*CHUNK_SZ, CHUNK_SZ, timeout);
            if (ret != CHUNK_SZ) {
                std::stringstream ss;
                ss << "didn't read appropriate chunk size: " << ret << " vs " << CHUNK_SZ << std::endl;
                log_err(ss.str());
            }
            if (ret < 0) return ret;
        }

        // transfer the rest
        if (leftover != 0) {
            ret = raw_funcs.bulk_transfer(han, endpoint, buffer + nchunks*CHUNK_SZ, leftover, timeout);
            if (ret != leftover) {
                std::stringstream ss;
                ss << "didn't read appropriate leftover: " << ret << " vs " << leftover << std::endl;
                log_err(ss.str());
            }
            if (ret < 0) return ret;
        }
        return 0;
    }
}
