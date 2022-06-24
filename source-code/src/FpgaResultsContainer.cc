#include "FpgaResultsContainer.hh"

namespace SipmUsb	
{
	FpgaRsltContainer::FpgaRsltContainer() :
        IoContainer<uint16_t, FPGA_RSLT_NUM_REGS>(){ }
        
        FpgaRsltContainer::~FpgaRsltContainer(){ }
        
        FpgaRsltContainer::ResultsAry FpgaRsltContainer::get_fpga_results() const	{
        	FpgaRsltContainer::Registers regs = registers_from_buffer(WhichBuffer::read);
        	FpgaRsltContainer::ResultsAry rslt;
        	
        	for(int i = 0; i < FPGA_RSLT_NUM_REGS; i++)	{
        		rslt[i] = regs[i];
        	}
        	
        	return rslt;
        	
        }
        
}
