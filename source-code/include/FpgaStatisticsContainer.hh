#pragma once

#include "IoContainer.hh"

namespace SipmUsb
{
	static const uint16_t FPGA_STAT_NUM_REGS = 16;
	static const uint8_t FPGA_STAT_CMD_ID = 1;

	class FpgaStatisticsContainer : public IoContainer<uint32_t, FGPA_STAT_NUM_REGS>
	{
	protected:
		McaUsbFlags mca_flags() const override
			{ return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, FPGA_STAT_CMD_ID}; }
	
	public:

		FpgaStatisticsContainer();
		~FpgaStatisticsContainer();
	};

}
