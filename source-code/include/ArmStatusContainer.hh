#pragma once

#include "IoContainer.hh"

namespace SipmUsb
{
	static const uint16_t ARM_STAT_NUM_REGS = 7;
	static const uint8_t ARM_STAT_CMD_ID = 1;
	
	class ArmStatusContainer : public IoContainer<float, ARM_STAT_NUM_REGS>
	{
        protected:
		McaUsbFlags mca_flags() const override
			{ return {ARM_READ_TYPE, ARM_WRITE_TYPE, ARM_STAT_CMD_ID}; }
        public:

		ArmStatusContainer();
		~ArmStatusContainer() { }
	};
	
}

