#include "main.hh"

int main(int argc, char* argv[])
{
    SipmUsb::LegacyHistogramContainer::HistoAry spectrum;
    try {
        spectrum = acquire_histogram();
    } catch (const std::runtime_error& e) {
        std::cout << "Error acquiring histogram: " << e.what() << std::endl;
        return -1;
    }

    size_t i = 0;
    for (const auto& counts_in_bin : spectrum) {
        std::cout << counts_in_bin << ' ';
        if (i != 0 && i % 8 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;
    // acquire_list();
    return 0;
}

void acquire_list()
{
    // nothin for now
}

// type definition in the corresponding .hh file
SipmUsb::LegacyHistogramContainer::HistoAry acquire_histogram()
{
    using namespace std::chrono_literals;
    // time is just locally checked so this isn't super accurate at the moment
    const auto COLLECTION_TIME = 5s;

    // to use the simulator instead, change the line to the SimManager one
    // everything should work the same (except updating settings is weird with the simulator)
    // SimManager and UsbManager are subclasses of BaseManager.
    /* SipmUsb::SimManager man("./lib/sipm_3k_simusb.so"); */
    SipmUsb::UsbManager man;

    // get the first serial number of a connected device (assume there is one connected)
    // if there isn't one connected, an exception will be thrown at some point
    std::string connected_serial;
    if (man.peek_serials().size() != 0) {
        connected_serial = man.peek_serials().at(0);
    }

    initialize_detector(man, connected_serial);

    // now read an actual histogram

    // configure commands
    SipmUsb::FpgaActionContainer fac;
    fac.update_transfer_flags(SipmUsb::MemoryType::ram, SipmUsb::TransferDirection::write);
    fac.update_write_args(fac.START_NEW_HISTOGRAM_ACQUISITION);

    // to change the number of bins requested, change LEGACY_HISTO_NUM_REGS in LegacyHistogramContainer.hh
    // it's called "legacy" because the custom firmware will supplant it
    SipmUsb::LegacyHistogramContainer lhc;
    lhc.update_transfer_flags(SipmUsb::MemoryType::ram, SipmUsb::TransferDirection::read);

    // send command to detector to start the measurement
    std::cout << "dispatching 'start measurement' command" << std::endl;
    man.write_from(connected_serial, fac);
    // wait for measurement to complete or poll the detector until it's done (haven't implemented the polling yet)
    std::this_thread::sleep_for(COLLECTION_TIME);
    
    // fill registers of container with histogram data
    man.read_into(connected_serial, lhc);
    std::cout << "done reading spectrum" << std::endl;

    // custom functionality of the histogram container
    return lhc.gen_binned_histogram();
}

template<class RegT, size_t NumRegs>
void setup_verify_one_cmd(
    SipmUsb::BaseManager& man,
    const std::string& sn,
    SipmUsb::IoContainer<RegT, NumRegs>& con,
    const typename SipmUsb::IoContainer<RegT, NumRegs>::Registers& regs)
{
    // write settings
    con.update_transfer_flags(SipmUsb::MemoryType::ram, SipmUsb::TransferDirection::write);
    con.update_write_args(regs);
    man.write_from(sn, con);

    // read back settings to verify
    con.update_transfer_flags(SipmUsb::MemoryType::ram, SipmUsb::TransferDirection::read);
    man.read_into(sn, con);
    cmp_registers(regs, con.registers_from_buffer(SipmUsb::WhichBuffer::read));

}

void initialize_detector(SipmUsb::BaseManager& man, const std::string& sn)
{
    // set fpga_ctrl and arm_ctrl settings that we want.
    // change these registers to the ones that you want in order to properly configure the device
    // the "Registers" type is an alias for std::array<RegisterType, NumRegisters>
    // that contains the correct RegisterType and NumRegisters
    
    // alias the typenames b/c they're really long
    using fcr = SipmUsb::FpgaCtrlContainer::Registers;
    using acr = SipmUsb::ArmCtrlContainer::Registers;
                         // these registers were "optimized" for EXACT FSR but will not align with CeBr3 needs
    fcr fpga_ctrl_regs = SipmUsb::FpgaCtrlContainer::LM_OPTIMIZED_REGISTERS;
    acr arm_ctrl_regs =  SipmUsb::ArmCtrlContainer::LM_OPTIMIZED_REGISTERS;

    // these objects are subclasses of IoContainer
    // they hold information relevant to bridgeport commands.
    // some IoContainer subclasses offer custom decoding
    SipmUsb::FpgaCtrlContainer fcc;
    SipmUsb::ArmCtrlContainer acc;

    setup_verify_one_cmd(man, sn, fcc, fpga_ctrl_regs);
    std::cout << "done fpga ctrl" << std::endl;
    setup_verify_one_cmd(man, sn, acc, arm_ctrl_regs);
    std::cout << "done arm ctrl" << std::endl;
}

template<class RegAry>
void cmp_registers(const RegAry& saved_regs, const RegAry& read_regs)
{
    for (size_t i = 0; i < read_regs.size(); ++i) {
        auto loaded = read_regs[i];
        auto saved = saved_regs[i];
        if (loaded != saved) {
            std::cout << "registers at index " << i << " are not equal" << std::endl
                      << "saved is: " << saved << std::endl
                      << "loaded is: " << loaded << std::endl;
        }
    }
}
