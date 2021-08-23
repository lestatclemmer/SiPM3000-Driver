#include "ListModeContainer.hpp"

namespace SipmUsb
{
    ListModeContainer::ListModeContainer() :
        IoContainer<uint16_t, LIST_MODE_NUM_REGS>()
    { }

    std::vector<ListModeDataPoint> ListModeContainer::parse_list_buffer() const
    {
        Registers regs = registers_from_buffer(WhichBuffer::read);
        std::vector<ListModeDataPoint> ret;
        uint16_t detail_reg = regs[0];
        // as per Bridgeport, first 12 bits hold # of events
        size_t num_events = detail_reg & 0xfff;
        // mode is the topmost bit
        uint8_t lm_mode = detail_reg >> 15;
        if (lm_mode == 1) { throw std::runtime_error("lm_mode must be set to zero for the higher time precision"); }

        // each list mode event is 3 uint16_t long. they start at index 4.
        size_t max_idx = 4 + 3*num_events;
        uint32_t ts_clock_cycles;
        uint16_t energy;

        for (size_t i = 4; i < max_idx; i += 3) {
            ts_clock_cycles = regs[i+1] | (regs[i+2] << 16);
            // throw away extra energy precision . . . for now
            energy = regs[i] / 16;
            ret.push_back({ts_clock_cycles, energy});
        }

        return ret;
    }
}
