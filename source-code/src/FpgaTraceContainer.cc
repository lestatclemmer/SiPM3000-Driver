#include "FpgaTraceContainer.hh"

namespace SipmUsb
{
	FpgaTraceContainer::FpgaTraceContainer() :
	IoContainer<int16_t, FPGA_TRACE_NUM_REGS>(){ }

	FpgaTraceContainer::~FpgaTraceContainer(){ }

}	
