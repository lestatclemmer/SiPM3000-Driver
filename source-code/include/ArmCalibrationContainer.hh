#pragma once

#include "IoContainer.hh"

namespace SipmUsb
{
	static const uint16_t ARM_CAL_NUM_REGS = 64;
	static const uint8_t ARM_CAL_CMD_ID = 3;
	
	class ArmCalibrationContainer : public IoContainer<float, ARM_CAL_NUM_REGS>
	{
        protected:
		McaUsbFlags mca_flags() const override
			{ return {ARM_READ_TYPE, ARM_WRITE_TYPE, ARM_CAL_CMD_ID}; }
        public:

		ArmCalibrationContainer();
		~ArmCalibrationContainer() { }
	};
	
}

