#pragma once

#include <cerrno>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include "usb.h"

namespace SipmUsb
{
    class UsbFuncException : public std::runtime_error
    {
        public:
            UsbFuncException(const char* what) : std::runtime_error(what)
            { }
            UsbFuncException(const std::string& what) : std::runtime_error(what.c_str())
            { }
    };

    class UsbFuncHandle
    {
        private:
            // from USB standard https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__endpoint__descriptor.html
            static const uint8_t XFER_DIR_FLAG = 1 << 7;
            // function types to hold libusb funcs
            typedef void (*init_t)(void);
            typedef int (*find_busses_t)(void);
            typedef int (*find_devices_t)(void);
            typedef usb_bus* (*get_busses_t)(void);
            typedef struct usb_dev_handle* (*open_t)(struct usb_device*);
            typedef int (*close_t)(usb_dev_handle*);
            typedef int (*det_kern_t)(usb_dev_handle*, int);
            typedef int (*interface_t)(usb_dev_handle*, int); 
            typedef int (*bulk_t)(usb_dev_handle*, int, char*, int, int);

            void* libusb_handle;
            // functions corresponding to what libusb has
            init_t raw_init;
            find_busses_t raw_find_busses;
            find_devices_t raw_find_devices;
            get_busses_t raw_get_busses;
            open_t raw_open;
            close_t raw_close;
            det_kern_t raw_detach_kernel_driver_np;
            interface_t raw_claim_interface;
            interface_t raw_release_interface;
            bulk_t raw_bulk_read;
            bulk_t raw_bulk_write;

            int verify_open(const char* sym);
            virtual void close() { if (libusb_handle) dlclose(libusb_handle); }

            // override for custom logging
            virtual void log_err(const std::string& emsg) { std::cerr << emsg << std::endl; }
        public:
            // no funny business
            explicit UsbFuncHandle(const std::string& lib_path);
            // no copying or default init (just for simplicity/safety with dlopen/dlclose)
            UsbFuncHandle() =delete;
            UsbFuncHandle& operator=(UsbFuncHandle& other) =delete;
            UsbFuncHandle(const UsbFuncHandle& other) =delete;
            // only need to clean up dynamic library handle
            virtual ~UsbFuncHandle() { close(); }

            // wrappers for the raw libusb funcs
            void init();
            int find_busses();
            int find_devices();
            struct usb_bus* get_busses();
            struct usb_dev_handle* open(struct usb_device* dev);
            int close(struct usb_dev_handle* dev); 
            int detatch_kernel_driver_np(struct usb_dev_handle* dev, int interface);
            int claim_interface(struct usb_dev_handle* dev, int interface);
            int release_interface(struct usb_dev_handle* dev, int interface);
            int bulk_read(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout);
            int bulk_write(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout);
            // discern read or write from the endpoint
            int bulk_transfer(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout);
    };
}
