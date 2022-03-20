#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <BaseManager.hh>
#include <SimManager.hh>
#include <UsbManager.hh>

#include <IoContainer.hh>
#include <ArmCtrlContainer.hh>
#include <FpgaCtrlContainer.hh>
#include <LegacyHistogramContainer.hh>
#include <FpgaActionContainer.hh>

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

