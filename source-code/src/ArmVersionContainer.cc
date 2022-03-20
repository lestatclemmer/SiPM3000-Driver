#include "ArmVersionContainer.hh"

namespace SipmUsb {
    ArmVersionContainer::ArmVersionContainer() :
        IoContainer<uint8_t, ARM_VERSION_NUM_REGS>()
    { }

    ArmVersionContainer::~ArmVersionContainer() { }

    std::string ArmVersionContainer::decode_serial_number() const
    {
        // build registers up from the raw buffers
        Registers regs(registers_from_buffer(WhichBuffer::read));
        std::stringstream ss;
        // location of serial number
        for (size_t i = 8; i < 24; ++i) {
            ss << std::hex << std::setw(2)
               << std::setfill('0') << std::uppercase
               << (0xff & regs[i]);
        }

        return ss.str();
    }
}
