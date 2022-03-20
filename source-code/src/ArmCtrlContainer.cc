#include "ArmCtrlContainer.hh"

namespace SipmUsb
{
    ArmCtrlContainer::ArmCtrlContainer() : 
        IoContainer<float, ARM_CTRL_NUM_REGS>()
    {
        update_write_args(LM_OPTIMIZED_REGISTERS);
    }
}
