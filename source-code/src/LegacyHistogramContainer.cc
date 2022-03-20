#include "LegacyHistogramContainer.hh"

namespace SipmUsb
{
    // default constructor initializes to nothing
    LegacyHistogramContainer::LegacyHistogramContainer() :
        IoContainer<uint32_t, SipmUsb::LEGACY_HISTO_NUM_REGS>()
    { }

    LegacyHistogramContainer::HistoAry LegacyHistogramContainer::gen_binned_histogram() const
    {
        LegacyHistogramContainer::Registers regs = registers_from_buffer(WhichBuffer::read);
        LegacyHistogramContainer::HistoAry ret;
        for (size_t i = 0; i < ret.size(); ++i) {
            ret[i] = static_cast<uint16_t>(regs[i]);
        }

        return ret;
    }
}
