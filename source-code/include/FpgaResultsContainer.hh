#pragma once

#include "IoContainer.hh"

namespace SipmUsb
{
	static const uint16_t FPGA_RSLT_NUM_REGS = 16;
	static const uint8_t FPGA_RSLT_CMD_ID = 2;
	
	class FpgaRsltContainer : public IoContainer<uint16_t, FPGA_RSLT_NUM_REGS>
	{
        protected:
		McaUsbFlags mca_flags() const override
			{ return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, FPGA_RSLT_CMD_ID}; }
        public:
        	using ResultsAry = std::array<uint16_t, FPGA_RSLT_NUM_REGS>;
        	
        	ResultsAry get_fpga_results() const;

		FpgaRsltContainer();
		~FpgaRsltContainer() { }
	};
	
}


