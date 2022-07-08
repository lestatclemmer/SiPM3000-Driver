#include "FpgaStatisticsContainer.hh"

namespace SipmUsb
{
	FpgaStatisticsContainer::FpgaStatisticsContainer():
		IoContainer<uint32_t, FPGA_STAT_NUM_REGS>()
	{ }

	FpgaStatisticsContainer::~FpgaStatisticsContainer(){ }

}
