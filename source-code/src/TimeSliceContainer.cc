#include "TimeSliceContainer.hh"

namespace SipmUsb
{
	TimeSliceContainer::TimeSliceContainer():
		IoContainer<uint16_t, TIME_SLICE_NUM_REGS>()
	{ }

	TimeSliceContainer::~TimeSliceContainer(){ }


}
