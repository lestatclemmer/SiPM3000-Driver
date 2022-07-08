#include "ArmCalibrationContainer.hh"

namespace SipmUsb	
{
	ArmCalibrationContainer::ArmCalibrationContainer() :
        IoContainer<float, ARM_CAL_NUM_REGS>(){ }
        
        ArmCalibrationContainer::~ArmCalibrationContainer(){ }
        
}
