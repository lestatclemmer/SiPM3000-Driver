#include "FpgaActionContainer.hpp"

namespace SipmUsb
{
    FpgaActionContainer::FpgaActionContainer() : 
        IoContainer<uint16_t, FPGA_ACTION_NUM_REGS>()
    {
        update_write_args(START_NEW_LIST_ACQUISITION);
    }
}
