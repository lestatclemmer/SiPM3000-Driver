#pragma once

#include <cstdint>
#include <array>
#include "IoContainer.hpp"

namespace SipmUsb
{
    static const uint16_t LEGACY_HISTO_NUM_REGS = 4096;
    static const uint8_t LEGACY_HISTO_CMD_ID = 3;
    class LegacyHistogramContainer : public IoContainer<uint32_t, LEGACY_HISTO_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, LEGACY_HISTO_CMD_ID}; }
        public:
            using HistoAry = std::array<uint16_t, LEGACY_HISTO_NUM_REGS>;
            LegacyHistogramContainer();
            ~LegacyHistogramContainer() { }
            HistoAry gen_binned_histogram() const;
    };
}
