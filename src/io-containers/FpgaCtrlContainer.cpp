#include "FpgaCtrlContainer.hpp"

namespace SipmUsb
{
    FpgaCtrlContainer::FpgaCtrlContainer() :
        IoContainer<uint16_t, FPGA_CTRL_NUM_REGS>()
    {
        // initialize with optimized settings
        update_write_args(LM_OPTIMIZED_REGISTERS);
    }
}
