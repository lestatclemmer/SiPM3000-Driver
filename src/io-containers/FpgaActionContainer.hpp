#pragma once

#include "IoContainer.hpp"

namespace SipmUsb
{
    static const uint8_t FPGA_ACTION_CMD_ID = 7;
    static const uint8_t FPGA_ACTION_NUM_REGS = 4;
    class FpgaActionContainer : public IoContainer<uint16_t, FPGA_ACTION_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, FPGA_ACTION_CMD_ID}; }
        public:
            // as per http://bridgeportinstruments.com/products/software/wxMCA_doc/documentation/english/mds/mca3k/mca3k_fpga_action.html
            static constexpr Registers START_NEW_LIST_ACQUISITION = {
                // bits mean:
                0b1111, // clear everything
                0,      // unused
                0b1000, // enable list mode
                0       // unused
            };

            static constexpr Registers START_NEW_HISTOGRAM_ACQUISITION = {
                // bits mean:
                0b1111, // clear everything
                0,      // unused
                0b0001, // enable histogram mode
                0       // unused
            };

            FpgaActionContainer();
            ~FpgaActionContainer() { }
    };
}
