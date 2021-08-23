#pragma once

/*
 * Note: some function bodies are included in here because of templates
 */

#include <cstring>
#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <libusb.h>
#include "usb.h"

#include "IoContainer.hpp"
#include "ArmVersionContainer.hpp"

namespace SipmUsb
{
    class ClaimInterfaceException : public std::runtime_error
    {
        public:
            ClaimInterfaceException(const char* what) : std::runtime_error(what) { }
    };
    class WriteBytesException : public std::runtime_error
    {
        public:
            WriteBytesException(const char* what) : std::runtime_error(what) { }
    };
    class ReadBytesException : public std::runtime_error
    {
        public:
            ReadBytesException(const char* what) : std::runtime_error(what) { }
    };
    class SerialNotFoundError : public std::runtime_error
    {
        public:
            SerialNotFoundError(const char* what) : std::runtime_error(what) { }
    };

    union LibUsbHandleWrap
    {
        struct usb_dev_handle* legacy_han; 
        libusb_device_handle*  new_han;
    };

    union LibUsbDeviceWrap
    {
        struct usb_device* legacy_dev;
        libusb_device*     new_dev;
    };


    class BaseManager
    {
        public:
            BaseManager() { }
            // no copying
            BaseManager(const BaseManager& other) =delete;
            BaseManager& operator=(BaseManager& other) =delete;
            // abstract base class
            virtual ~BaseManager() =0;

            // look at the serial numbers
            std::vector<std::string> peek_serials() const
            {
                std::vector<std::string> ret;
                for (const auto& p : dev_map) ret.push_back(p.first);
                return ret;
            }

            // writes settings or data from the IoContainer
            template<class RegT, size_t NumRegs>
            void write_from(const std::string& arm_serial, IoContainer<RegT, NumRegs>& con)
            {
                verify_rw_params(arm_serial, con);
                LibUsbHandleWrap han_wrap = dev_map[arm_serial];

                int xfer_ret = xfer_in_chunks(
                    han_wrap, BaseManager::CMD_OUT_EP, con.cmd_buffer,
                    con.NUM_CMD_WRITE_BYTES, BaseManager::TIMEOUT_MS);

                if (xfer_ret < 0) {
                    std::stringstream ss;
                    ss << "Error writing cmd to handle specified by SN " << arm_serial;
                    throw WriteBytesException(ss.str().c_str());
                }

                // if there is more data to write to the device (in the write data buffer)
                if (!con.short_write_possible()) {
                    xfer_ret = xfer_in_chunks(
                        han_wrap, BaseManager::DATA_OUT_EP, con.write_data_buffer,
                        con.NUM_DATA_BYTES, BaseManager::TIMEOUT_MS);
                    if (xfer_ret < 0) {
                        std::stringstream ss;
                        ss << "Error writing additional data to handle specified by SN " << arm_serial;
                        throw WriteBytesException(ss.str().c_str());
                    }
                }
            }

            // reads data into the IoContainer
            template<class RegT, size_t NumRegs>
            void read_into(const std::string& arm_serial, IoContainer<RegT, NumRegs>& con)
            {
                verify_rw_params(arm_serial, con);
                LibUsbHandleWrap han_wrap = dev_map[arm_serial];
                // send command saying, "hi, i want data"
                int xfer_ret = xfer_in_chunks(
                    han_wrap, BaseManager::CMD_OUT_EP, con.cmd_buffer,
                    con.NUM_CMD_WRITE_BYTES, BaseManager::TIMEOUT_MS);

                if (xfer_ret < 0) {
                    std::stringstream ss;
                    ss << "Error writing cmd to handle specified by SN " << arm_serial;
                    throw WriteBytesException(ss.str().c_str());
                }

                // read the actual data now
                xfer_ret = xfer_in_chunks(
                    han_wrap, BaseManager::DATA_IN_EP, con.read_data_buffer,
                    con.NUM_DATA_BYTES, BaseManager::TIMEOUT_MS);

                if (xfer_ret < 0) {
                    std::stringstream ss;
                    ss << "Error reading data from handle specified by SN " << arm_serial;
                    throw ReadBytesException(ss.str().c_str());
                }
                // done!
            }

        protected:
            static const int TIMEOUT_MS = 1000;
            // from Bridgeport code
            static const int DETECTOR_INTERFACE = 1;
            static const uint16_t BRIDGEPORT_VID = 0x1fa4;
            static const int CMD_OUT_EP = 0x01;
            static const int CMD_IN_EP = 0x81;
            static const int DATA_OUT_EP = 0x02;
            static const int DATA_IN_EP = 0x82;

            // exposed usb interface
            std::map<std::string, LibUsbHandleWrap> dev_map;
            // temporary hold on the handles before mapping
            std::vector<LibUsbHandleWrap> devices;

            virtual void claim_bridgeport_devices() =0;
            virtual void claim_single_bp_dev(LibUsbDeviceWrap& dev_wrap) =0;
            virtual int xfer_in_chunks(
                LibUsbHandleWrap& han, int endpoint, void* raw_buffer, int num_bytes, int timeout) =0;

            // same in all cases
            std::string extract_bp_sn(LibUsbHandleWrap& han_wrap);
            void map_bridgeport_devices();

            template<class RegT, size_t NumRegs>
            void verify_rw_params(std::string arm_serial, const IoContainer<RegT, NumRegs>& con) const
            {
                if (dev_map.count(arm_serial) == 0) {
                    std::stringstream ss;
                    ss << "Can't find serial number '" << arm_serial << "' in device map";
                    throw SerialNotFoundError(ss.str().c_str());
                }

                auto wbuf_ptr = con.write_data_buffer;
                if (std::equal(wbuf_ptr, wbuf_ptr + con.NUM_DATA_BYTES, wbuf_ptr)) {
                    // do nothing
                    // log_err("Warning: registers to write in are empty. This is ok for some commands.");
                }
            }

            // override for custom logging
            virtual void log_err(const std::string& emsg) const { std::cerr << emsg << std::endl; }
    };
}
