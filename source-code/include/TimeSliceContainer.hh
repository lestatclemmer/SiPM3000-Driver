#pragma once

namespace SipmUsb
{
    const uint8_t TIME_SLICE_CMD_ID = 8;
    const uint16_t TIME_SLICE_NUM_REGS = 1024;
    class TimeSliceContainer : public IoContainer<uint16_t, TIME_SLICE_NUM_REGS>
    {
        protected:
            McaUsbFlags mca_flags() const override
            { return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, TIME_SLICE_CMD_ID}; }
        public:
            // TODO: do this part
	    TimeSliceContainer();
	    ~TimeSliceContainer();

    };
}
