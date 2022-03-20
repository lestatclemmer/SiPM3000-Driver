#pragma once

#include "IoContainer.hh"

namespace SipmUsb
{
    static const uint16_t FPGA_CTRL_NUM_REGS = 16;
    static const uint8_t FPGA_CTRL_CMD_ID = 0;
    class FpgaCtrlContainer : public IoContainer<uint16_t, FPGA_CTRL_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, FPGA_CTRL_CMD_ID}; }
        public:
            // computed using mca_api.py and mca3k_data.py, as well as a saved JSON file of optimized settings
            static constexpr Registers LM_OPTIMIZED_REGISTERS = {
                17800, 20, 34, 72, 60, 65280, 100, 1092, 0, 0, 0, 0, 3906, 0, 33008, 32768
            };
            FpgaCtrlContainer();
            ~FpgaCtrlContainer() { }
    };
}
