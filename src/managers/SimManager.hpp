#pragma once

/*
 * Note: some function bodies are included in here because of templates
 */

#include <chrono>
#include <map>
#include <string>
#include <thread>

#include "BaseManager.hpp"
#include "UsbFuncHandle.hpp"
// very old libusb used by simulator
#include "usb.h"

namespace SipmUsb
{
    class SimManager : public BaseManager
    {
        public:
            explicit SimManager(const std::string& lib_path);
            // no copying
            SimManager() =delete;
            SimManager(const SimManager& other) =delete;
            SimManager& operator=(SimManager& other) =delete;
            // need destructor to close devices properly
            virtual ~SimManager();
        private:
            // very old libusb functions used by simulator
            UsbFuncHandle raw_funcs;

            void claim_bridgeport_devices() override;
            void claim_single_bp_dev(LibUsbDeviceWrap& dev_wrap) override;
            void ping_devices();

            int xfer_in_chunks(
                LibUsbHandleWrap& han_wrap, int endpoint, void* buffer, int num_bytes, int timeout) override;
    };
}
