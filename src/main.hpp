#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <BaseManager.hpp>
#include <SimManager.hpp>
#include <UsbManager.hpp>

#include <IoContainer.hpp>
#include <ArmCtrlContainer.hpp>
#include <FpgaCtrlContainer.hpp>
#include <LegacyHistogramContainer.hpp>
#include <FpgaActionContainer.hpp>

SipmUsb::LegacyHistogramContainer::HistoAry acquire_histogram();
void acquire_list();
template<class RegAry>
void cmp_registers(const RegAry& saved_regs, const RegAry& read_regs);

template<class RegT, size_t NumRegs>
void setup_verify_one_command(
    SipmUsb::BaseManager&, 
    const std::string&,
    SipmUsb::IoContainer<RegT, NumRegs>&,
    const typename SipmUsb::IoContainer<RegT, NumRegs>::Registers&);

void initialize_detector(SipmUsb::BaseManager&, const std::string&);

