#pragma once

#include <array>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>
#include <utility>
#include <cstdint>

/*
Note: we need function implementations inside of this header so that
the linker knows what's going on.

See: https://stackoverflow.com/questions/8752837/undefined-reference-to-template-class-constructor
*/

// C++17 required for static constexpr inside of a class
#if __cplusplus < 201703L
    #error "at least -std=c++17 is required to properly compile SipmUsb::IoContainer"
#endif

namespace SipmUsb
{
    static const uint8_t FPGA_WRITE_TYPE = 1;
    static const uint8_t FPGA_READ_TYPE = 2;
    static const uint8_t ARM_WRITE_TYPE = 3;
    static const uint8_t ARM_READ_TYPE = 4;
    // ripped from mca3k_device.py
    // numbers that tell the detector what command it will receive
    struct McaUsbFlags
    {
        uint8_t read_type;
        uint8_t write_type;
        uint8_t command_ident;
    };

    enum class TransferDirection
    {
        write,
        read
    };

    enum class MemoryType { ram, nvram };
    enum class WhichBuffer { read, write };

    // registers can be differend widths (uint16_t, uint32_t, for example)
    // we want to returnn different data based on what kind of command we use (List, Histogram, etc.)
    template<typename RegT, size_t NumRegs>
    class IoContainer
    {
        public:
            using Registers = std::array<RegT, NumRegs>;

            // no copying
            IoContainer(const IoContainer& other) =delete;
            IoContainer& operator=(const IoContainer& other) =delete;

            // this is bad practice to expose data like this
            // but just don't mess with it lol
            static constexpr size_t NUM_CMD_WRITE_BYTES = 64;
            static constexpr size_t NUM_DATA_BYTES = sizeof(RegT) * NumRegs;
            unsigned char* cmd_buffer_ptr()
            { return cmd_buffer.data(); }
            const unsigned char* cmd_buffer_ptr() const
            { return cmd_buffer.data(); }

            unsigned char* read_data_buffer_ptr()
            { return read_data_buffer.data(); }
            const unsigned char* read_data_buffer_ptr() const
            { return read_data_buffer.data(); }

            unsigned char* write_data_buffer_ptr()
            { return write_data_buffer.data(); }
            const unsigned char* write_data_buffer_ptr() const
            { return write_data_buffer.data(); }

            static bool short_write_possible() { return NUM_CMD_WRITE_BYTES - 4 >= NUM_DATA_BYTES; }

            void update_write_args(const Registers& new_regs)
            {
                // fill the write buffer with user-provided registers
                // can't use memcpy bc it has no template definitions or something, idk
                const auto new_data_ptr = reinterpret_cast<const unsigned char*>(new_regs.data());
                for (size_t i = 0; i < NUM_DATA_BYTES; ++i) {
                    write_data_buffer[i] = new_data_ptr[i];
                }

                if (short_write_possible()) {
                    // clear the old short write data out (if it's there)
                    unsigned char* sw_buf = cmd_buffer.data() + 4;
                    // copy write buffer data into the command packet to send all at once 
                    for (size_t i = 0; i < NUM_CMD_WRITE_BYTES; ++i) {
                        sw_buf[i] = (i < NUM_DATA_BYTES)? new_data_ptr[i] : 0;
                    }
                }
            }

            void update_transfer_flags(const MemoryType& mt, const TransferDirection& dir)
            {
                uint32_t xfer_flags = 0;
                const McaUsbFlags f = mca_flags();
                uint32_t nbytes;

                switch(dir) {
                    case TransferDirection::write:
                        xfer_flags += f.write_type;
                        // if we can squeeze the whole command into one 60-byte chunk, do it
                        if (short_write_possible()) {
                            // indicate we are squeezing command + data into one packet (mca_device.py)
                            xfer_flags += SHORT_WRITE_FLAG;
                        }
                        nbytes = NUM_CMD_WRITE_BYTES;
                        break;
                    case TransferDirection::read:
                    default:
                        xfer_flags = f.read_type;
                        nbytes = NUM_DATA_BYTES;
                        break;
                }

                // 0 = ram, 1 = nvram
                uint32_t mt_int = (mt == MemoryType::ram)? 0 : 1;
                uint32_t head =
                    (nbytes << 16) +
                    (mt_int << 12) +
                    (f.command_ident << 4) +
                    xfer_flags;

                // put the header at the start of the command buffer
                auto head_p = reinterpret_cast<unsigned char*>(&head);
                for (size_t i = 0; i < 4; ++i)
                    cmd_buffer[i] = head_p[i];
            }

            // Construct registers of the correct data type using the user-provided buffer choice.
            Registers registers_from_buffer(WhichBuffer b) const
            {
                Registers ret;

                auto ret_ptr = reinterpret_cast<unsigned char*>(ret.data());
                const auto& buf = (b == WhichBuffer::read)? read_data_buffer : write_data_buffer;

                for (size_t i = 0; i < NUM_DATA_BYTES; ++i) {
                    ret_ptr[i] = buf[i];
                }

                return ret;
            }
        protected:
            std::array<unsigned char, NUM_CMD_WRITE_BYTES> cmd_buffer;
            std::array<unsigned char, NUM_DATA_BYTES> read_data_buffer;
            // note: the detector expects a packet of at least 64 bytes.
            // if there are fewer bytes than this, the detector might crash.
            // however, the only way that fewer than 64 bytes may be sent to the detector at the moment
            // is via a short write, meaning that the data bytes get packed into the command packet
            // and sent all at once. so there is no harm in only allocating NUM_DATA_BYTES to the write buffer,
            // even if NUM_DATA_BYTES < 64. they will never be written on their own.
            std::array<unsigned char, NUM_DATA_BYTES> write_data_buffer;

            // generic class has no flags; make this pure virtual.
            virtual McaUsbFlags mca_flags() const =0;

            IoContainer() :
                cmd_buffer(),
                read_data_buffer(),
                write_data_buffer()
            { }

            virtual ~IoContainer()
            { }
        private:
            // from mca_device.py
            const uint32_t SHORT_WRITE_FLAG = 0x800;
            /* // so it can access pointers */
            /* friend class BaseManager; */
    };
}
