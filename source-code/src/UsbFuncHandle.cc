#include "UsbFuncHandle.hh"

namespace SipmUsb
{
    UsbFuncHandle::UsbFuncHandle(const std::string& lib_path) :
        libusb_handle(dlopen(lib_path.c_str(), RTLD_LAZY))
    {
        if (!libusb_handle) {
            std::string err = "Can't open the libusb simulator .so dynamic library: \n";
            throw UsbFuncException(err + dlerror());
        }
        int ret = 0;

        // |= so that if any load fails, ret is 1
        raw_init = (init_t) (dlsym(libusb_handle, "usb_init"));
        ret |= verify_open("usb_init"); 

        raw_find_busses = (find_busses_t) (dlsym(libusb_handle, "usb_find_busses"));
        ret |= verify_open("usb_find_busses"); 

        raw_find_devices = (find_devices_t) (dlsym(libusb_handle, "usb_find_devices"));
        ret |= verify_open("usb_find_devices"); 

        raw_get_busses = (get_busses_t) (dlsym(libusb_handle, "usb_get_busses"));
        ret |= verify_open("usb_get_busses"); 

        raw_open = (open_t) (dlsym(libusb_handle, "usb_open"));
        ret |= verify_open("usb_open"); 

        raw_close = (close_t) (dlsym(libusb_handle, "usb_close"));
        ret |= verify_open("usb_close");

        raw_detach_kernel_driver_np = (det_kern_t) (dlsym(libusb_handle, "usb_detach_kernel_driver_np"));
        ret |= verify_open("usb_detach_kernel_driver_np");
        
        raw_claim_interface = (interface_t) (dlsym(libusb_handle, "usb_claim_interface"));
        ret |= verify_open("usb_claim_interface");

        raw_release_interface = (interface_t) (dlsym(libusb_handle, "usb_claim_interface"));
        ret |= verify_open("usb_claim_interface");

        raw_bulk_read = (bulk_t) (dlsym(libusb_handle, "usb_bulk_read"));
        ret |= verify_open("usb_bulk_read");

        raw_bulk_write = (bulk_t) (dlsym(libusb_handle, "usb_bulk_write"));
        ret |= verify_open("usb_bulk_write");

        if (ret != 0) {
            // clean up dynamic library
            close();
            throw UsbFuncException("Failed to load a USB function. hmm...");
        }
    }

    int UsbFuncHandle::verify_open(const char* sym)
    {
        const char* err = dlerror();
        if (err) {
            std::stringstream ss;
            ss << "Error loading symbol " << sym << ": " << err;
            log_err(ss.str());
            return 1;
        }
        return 0;
    }

    // clump of wrappers functions. yucky
    void UsbFuncHandle::init()
    { return raw_init(); }

    int UsbFuncHandle::find_busses()
    { return raw_find_busses(); }

    int UsbFuncHandle::find_devices()
    { return raw_find_devices(); }

    struct usb_bus* UsbFuncHandle::get_busses()
    { return raw_get_busses(); }

    struct usb_dev_handle* UsbFuncHandle::open(struct usb_device* dev)
    { return raw_open(dev); }

    int UsbFuncHandle::close(struct usb_dev_handle* dev)
    { return raw_close(dev); }

    int UsbFuncHandle::detatch_kernel_driver_np(struct usb_dev_handle* dev, int interface)
    { return raw_detach_kernel_driver_np(dev, interface); }

    int UsbFuncHandle::claim_interface(struct usb_dev_handle* dev, int interface)
    { return raw_claim_interface(dev, interface); }

    int UsbFuncHandle::release_interface(struct usb_dev_handle* dev, int interface)
    { return raw_release_interface(dev, interface); }

    int UsbFuncHandle::bulk_read(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout)
    { return raw_bulk_read(dev, endpoint, buffer, num_bytes, timeout); }

    int UsbFuncHandle::bulk_write(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout)
    { return raw_bulk_write(dev, endpoint, buffer, num_bytes, timeout); }

    int UsbFuncHandle::bulk_transfer(struct usb_dev_handle* dev, int endpoint, char* buffer, int num_bytes, int timeout)
    {
        // discern read or write from the endpoint
        if (endpoint & UsbFuncHandle::XFER_DIR_FLAG) {
            return bulk_read(dev, endpoint, buffer, num_bytes, timeout);
        }
        else {
            return bulk_write(dev, endpoint, buffer, num_bytes, timeout);
        }
    }
}
