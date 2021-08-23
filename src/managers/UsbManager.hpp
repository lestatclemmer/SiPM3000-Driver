#pragma once

#include <libusb.h>
#include <map>
#include <string>
#include <vector>
#include "BaseManager.hpp"

namespace SipmUsb
{
    class UsbManager : public BaseManager
    {
        public:
            explicit UsbManager();
            // no copying
            UsbManager(const UsbManager& other) =delete;
            UsbManager& operator=(UsbManager& other) =delete;
            // need destructor to close devices properly
            virtual ~UsbManager();
        private:
            // new libusb-specific stuff
            libusb_context* usb_ctx;
            libusb_device** raw_devs;

            void claim_bridgeport_devices() override;
            void claim_single_bp_dev(LibUsbDeviceWrap& dev_wrap) override;

            int xfer_in_chunks(
                LibUsbHandleWrap& han_wrap, int endpoint, void* buffer, int num_bytes, int timeout) override;
    };
}
