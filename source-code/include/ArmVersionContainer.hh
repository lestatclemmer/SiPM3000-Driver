#pragma once

#include "IoContainer.hh"
#include <sstream>
#include <string>

namespace SipmUsb
{
    // other ARM constants defined in SipmUsb.h
    static const uint8_t ARM_VERSION_NUM_REGS = 64;
    static const uint8_t ARM_VERSION_CMD_ID = 0;
    class ArmVersionContainer : public IoContainer<uint8_t, ARM_VERSION_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {ARM_READ_TYPE, ARM_WRITE_TYPE, ARM_VERSION_CMD_ID}; }
        public:
            ArmVersionContainer();
            ~ArmVersionContainer();
            std::string decode_serial_number() const;
            // . . . add more functions as needed
    };
}
