#include "BaseManager.hpp"

namespace SipmUsb
{
    BaseManager::~BaseManager() { }

    void BaseManager::map_bridgeport_devices()
    {
        for (auto& han_wrap : devices) {
            std::string arm_serial_number = extract_bp_sn(han_wrap);
            dev_map[arm_serial_number] = han_wrap;
        }
    }

    std::string BaseManager::extract_bp_sn(LibUsbHandleWrap& han_wrap)
    {
        // don't put anything in registers
        ArmVersionContainer armvc;
        armvc.update_transfer_flags(MemoryType::ram, TransferDirection::read);

        int ret = xfer_in_chunks(
            han_wrap, BaseManager::CMD_OUT_EP, armvc.cmd_buffer, armvc.NUM_CMD_WRITE_BYTES, BaseManager::TIMEOUT_MS);
        if (ret != 0) {
            throw WriteBytesException("Read inappropriate # of bytes from USB");
        }

        ret = 0;
        ret = xfer_in_chunks(
            han_wrap, BaseManager::DATA_IN_EP, armvc.read_data_buffer, armvc.NUM_DATA_BYTES, BaseManager::TIMEOUT_MS);;
        if (ret < 0) {
            throw ReadBytesException("Problem reading serial number from detector");
        }

        // the magic
        return armvc.decode_serial_number();
    }
}
