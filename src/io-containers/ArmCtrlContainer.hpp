#pragma once

#include "IoContainer.hpp"

namespace SipmUsb
{
   static const uint8_t ARM_CTRL_CMD_ID = 2;
   /* Bridgeport website says arm_ctrl has 12 registers.
    * their code says otherwise.
    * . . .
    * i'm gonna leave it as 12 registers and see if it breaks anything.
    * w setterberg 19 august 2021
    */
   /* need it as 16 i guess (21 august 2021) */
   static const size_t ARM_CTRL_NUM_REGS = 16;

   class ArmCtrlContainer : public IoContainer<float, ARM_CTRL_NUM_REGS>
   {
        protected:
            McaUsbFlags mca_flags() const override
            { return {ARM_READ_TYPE, ARM_WRITE_TYPE, ARM_CTRL_CMD_ID}; }
        public:
            // computed using extract_registers.py
            static constexpr Registers LM_OPTIMIZED_REGISTERS = {
                0.0, 0.0, 1.0, 27.0, 1.0, 0.10000000149011612, 25.0, 34.0, 4800.0, 20000.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
            };
            ArmCtrlContainer();
            ~ArmCtrlContainer() { } 
   };
}
