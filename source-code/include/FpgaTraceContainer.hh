#pragma once

#include "IoContainer.hh"

namespace SiprmUsb
{
	static const uint16_t FPGA_TRACE_NUM_REGS = 1024;
	static const uint8_t FPGA_TRACE_CMD_ID = 4;

	class FpgaTraceContainer : public IoContainer<int16_t, FPGA_TRACE_NUM_REGS>
	{
	protected:
		McaUsbFlags mca_flags() const override
			{ return {FPGA_READ_TYPE, FPGA_WRITE_TYPE, FPGA_TRACE_CMD_ID}; }
	
	public:
		FpgaTraceContainer();
		~FpgaTraceContainer();

	};

}

