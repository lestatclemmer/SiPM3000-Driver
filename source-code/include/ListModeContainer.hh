#pragma once

#include <vector>
#include <iostream>
#include "IoContainer.hh"

namespace SipmUsb
{
    static const uint16_t LIST_MODE_NUM_REGS = 1024;
    static const uint8_t LIST_MODE_CMD_ID = 5;

    struct ListModeDataPoint
    {
        uint32_t rel_ts_clock_cycles;
        uint16_t energy_bin;
    };

    class ListModeContainer : public IoContainer<uint16_t, LIST_MODE_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, LIST_MODE_CMD_ID}; }
        public:
            // detector ADC clock speed in Hz
            static constexpr float CLOCK_SPEED = 40e6;

            ListModeContainer();
            ~ListModeContainer() { } // nothing to delete here

            std::vector<ListModeDataPoint> parse_list_buffer() const;
    };
}
