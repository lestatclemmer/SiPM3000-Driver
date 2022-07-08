#include "ArmStatusContainer.hh"

namespace SipmUsb	
{
	ArmStatusContainer::ArmStatusContainer() :
        IoContainer<float, ARM_STAT_NUM_REGS>(){ }
        
        ArmStatusContainer::~ArmStatContainer(){ }
        
}
