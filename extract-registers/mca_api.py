# release public
from __future__ import division
import time
import math
import os.path
import sys
import json
import array
import struct
import counter_data
import mca1k_data
import mca3k_data
import emorpho_data
import neutron3k_data
import counter3k_data

"""
This is the API layer for communication with a radiation detector with a bpi_usb USB interface.
It exchanges data with the driver layer exclusively via byte arrays.  The write and read function in this file are the 
switchboard where data are interpreted and the data exchange protocol is being enforced.

The data exchange is performed via a data port.  Sending header data to the bpi_usb.COMMAND_EP programs the data port 
and bulk data sent to the bpi_usb.DATA_EP will then be routed to the correct destination.  By the same token, 
preparing a read is also done by first sending data to the data port.  The ARM MCU then prepares the first buffer 
of data before the client sends the USB read command.
"""

"""
    Command structure:
    {
        'sn': ['sM1234'],  # serial number or list of serial numbers; if missing or empty list: do all units
        'dir': 'read',  # read, short_write,  write, rmw; 
        'mem': 'ram',  # ram or flash (flash = non-volatile memory)
        'type': 'sm_cmd',  # name space of the command
        'name': 'fpga_ctrl',  # name of the command
        'num_items': 16,  # To read a non-default number of items
        'ctrl': [],  # Control data written to mca.ctrl_out; only for FPGA control or action registers 
        'data': []   # Data longer than 62 bytes.
    }
    
"""


def read_config(file_name):
    """ Read a JSON file, remove Python style comments and return a dictionary
    Arguments: 
        file_name: Name of a configuration file in JSON format
    Returns:
        the dictionary decoded from the file content
    Raises:
        none
    """

"""
    write_any and read_any are the two general purpose I/O functions that communicate with the peripheral via USB.
    Any changes in the communication interface programmed into the device ARM M0+ will have to be reflected in 
    these two functions.
    All higher level functions are convenience functions for the benefit of the user.
"""

item_size_dict = {'H': 2, 'I': 4, 'i': 4, 'f': 4}  # Bytes per data item, depending on the data_type


def process_cmd(user_cmd, mca_dict):
    """
    A user_cmd is a dictionary with keys as shown above

    :param user_cmd:  A dictionary of command data and controls
    :param mca_dict:  A dictionary of mca objects; keys are the unique, and immutable, serial numbers
    :return: Dictionary of read-back data with serial numbers as the keys

    """
    cmd = {
        # serial number or list of serial numbers; if missing or empty list: do all units
        'dir': 'read',  # read, short_write,  write, rmw;
        'memory': 'ram',  # ram or flash (flash = non-volatile memory)
        'name': 'fpga_ctrl',  # name of the command
        'num_items': 0,  # To read a non-default number of items
        'data': {'registers': [], 'fields': [], 'user': []}   # Data to be routed via the data endpoints.
    }
    cmd.update(user_cmd)  # Update with data from the user command

    # Create a list of mca on which to perform the command
    # user_cmd['sn'] can be missing => perform on all mca
    # user_cmd['sn'] can be a string => perform on that mca
    # user_cmd['sn'] can be an empty list => perform on all mca
    # user_cmd['sn'] can be a list of strings => perform on all mca in the list
    mca_action = {}  # Dictionary of mca on which to perform the command.
    if 'sn' not in cmd:  # sn omitted => perform command on all mca devices.
        mca_action = mca_dict
    elif isinstance(cmd['sn'], str) and len(cmd['sn'])>0:  # Single serial number as a string
        mca_action[cmd['sn']] = mca_dict[cmd['sn']]
    elif isinstance(cmd['sn'], list):  # List of multiple serial numbers (must be strings)
        if len(cmd['sn']) == 0:
            mca_action = mca_dict  # sn omitted => perform command on all mca devices.
        else:
            for sn in cmd['sn']:
                mca_action[sn] = mca_dict[sn]

    # If there is a user_cmd['ctrl'] list, copy its first 30 values into cmd_out_list
    # These would be non-standard controls for cmd_out
    cmd_out_list = [0]*30  # 32 uint16_t integers; ie data_type='H'
    if "ctrl" in cmd:  # user_cmd['ctrl'] are always 16-bit integers
        if isinstance(cmd['ctrl'], list):
            num_ctrl = min(len(cmd['ctrl']), 30)
            ctrl = cmd['ctrl']
            for n in range(num_ctrl):
                cmd_out_list[n] = int(ctrl[n]) & 0xFFFF

    data_out_dict = {}
    data_out_list = []
    if cmd['dir'] == "write" or cmd['dir'] == "short_write":  # A write or short_write command only takes a list as data
        data_out_list = cmd['data']['registers']
    # A rmw command takes a dictionary with 'user' and 'fields' keys, but no 'registers'
    elif cmd['dir'] in ["rmw", "fields_to_user", "user_to_fields"]:
        data_out_dict = cmd['data']

    for sn in mca_action:
        mca_action[sn].cmd = cmd
        mca_action[sn].cmd_out_list = cmd_out_list
        mca_action[sn].data_out_dict = data_out_dict
        mca_action[sn].data_out_list = data_out_list

    in_dict = {}
    for sn in mca_action:
        mca = mca_action[sn]
        if mca.mca_id == 0x6001:
            ftdi_api().perform_cmd(mca)
        else:
            arm_api().perform_cmd(mca)
        in_dict[sn] = mca_action[sn].data_in_dict

    return in_dict

class arm_api():
    def __init__(self):
        """ List of command targets
            Objects will support read, short_write, write, rmW as appropriate
            Objects will support cmd_meme='flash' as appropriate.
            mca1k denotes devices without an FPGA
            mca3k is for devices with an FPGA and a waveform digitizing ADC.
            Devices with an ARM processor are preferred.
        """
        self.make_new_io_counter = {
            'arm_ping': counter_data.arm_ping,
            'arm_version': counter_data.arm_version,
            'arm_status': counter_data.arm_status,
            'arm_ctrl': counter_data.arm_ctrl,
            'arm_cal': counter_data.arm_cal,
            'arm_histogram': counter_data.arm_histogram,
            'arm_histo_2k': counter_data.arm_histo_2k,
            'arm_logger': counter_data.arm_logger
        }
        self.make_new_io_counter3k = {
            'arm_ping': counter3k_data.arm_ping,
            'arm_version': counter3k_data.arm_version,
            'arm_status': counter3k_data.arm_status,
            'arm_ctrl': counter3k_data.arm_ctrl,
            'arm_cal': counter3k_data.arm_cal,
            'fpga_ctrl': counter3k_data.fpga_ctrl,
            'fpga_action': counter3k_data.fpga_action,
            'fpga_statistics': counter3k_data.fpga_statistics,
        }
        self.make_new_io_obj_1k = {
            'arm_ping': mca1k_data.arm_ping,
            'arm_version': mca1k_data.arm_version,
            'arm_status': mca1k_data.arm_status,
            'arm_ctrl': mca1k_data.arm_ctrl,
            'arm_cal': mca1k_data.arm_cal,
            'arm_histogram': mca1k_data.arm_histogram,
            'arm_histo_2k': mca1k_data.arm_histo_2k,
            'arm_bck': mca1k_data.arm_bck,
            'arm_diff': mca1k_data.arm_diff,
            'arm_logger': mca1k_data.arm_logger
        }
        self.make_new_io_obj_3k = {
            'arm_ping': mca3k_data.arm_ping,
            'fpga_ctrl': mca3k_data.fpga_ctrl,
            'fpga_action': mca3k_data.fpga_action,
            'fpga_statistics': mca3k_data.fpga_statistics,
            'fpga_results': mca3k_data.fpga_results,
            'fpga_histogram': mca3k_data.fpga_histogram,
            'fpga_list_mode': mca3k_data.fpga_list_mode,
            'fpga_lm_nrl1': mca3k_data.fpga_lm_nrl1,
            'fpga_trace': mca3k_data.fpga_trace,
            'fpga_weights': mca3k_data.fpga_weights,
            'fpga_time_slice': mca3k_data.fpga_time_slice,
            'arm_version': mca3k_data.arm_version,
            'arm_status': mca3k_data.arm_status,
            'arm_ctrl': mca3k_data.arm_ctrl,
            'arm_cal': mca3k_data.arm_cal
        }
        
        self.make_new_io_obj_n3k = {  # neutron detector
            'arm_ping': neutron3k_data.arm_ping,
            'fpga_ctrl': neutron3k_data.fpga_ctrl,
            'fpga_action': neutron3k_data.fpga_action,
            'fpga_statistics': neutron3k_data.fpga_statistics,
            'fpga_results': neutron3k_data.fpga_results,
            'fpga_histogram': neutron3k_data.fpga_histogram,
            'fpga_trace': neutron3k_data.fpga_trace,
            'fpga_weights': neutron3k_data.fpga_weights,
            'arm_version': neutron3k_data.arm_version,
            'arm_status': neutron3k_data.arm_status,
            'arm_ctrl': neutron3k_data.arm_ctrl,
            'arm_cal': neutron3k_data.arm_cal,
            'arm_logger': neutron3k_data.arm_logger
        }
        
        self.make_new_io_obj_counter3k = {  # Digital pulse counter with ARM and FPGA
            'arm_ping': counter3k_data.arm_ping,
            'fpga_ctrl': counter3k_data.fpga_ctrl,
            'fpga_action': counter3k_data.fpga_action,
            'fpga_statistics': counter3k_data.fpga_statistics,
            'fpga_results': counter3k_data.fpga_results,
            'arm_version': counter3k_data.arm_version,
            'arm_status': counter3k_data.arm_status,
            'arm_ctrl': counter3k_data.arm_ctrl
        }
        
    def perform_cmd(self, mca):
        """
        Create a local io_obj according to the command name and execute the command.
        :param mca: An mca object describes one PMT-MCA
        :return: None
        """

        if mca.mca_id in [0x100, 0x200]:
            io_obj = self.make_new_io_counter[mca.cmd["name"]]()
            
        if mca.mca_id in [0x101, 0x201]:
            io_obj = self.make_new_io_obj_1k[mca.cmd["name"]]()
            
        if mca.mca_id in [0x103, 0x203]:
            io_obj = self.make_new_io_obj_3k[mca.cmd["name"]]()
            
        if mca.mca_id in [0x104, 0x204]:
            io_obj = self.make_new_io_obj_n3k[mca.cmd["name"]]()
            
        if mca.mca_id in [0x005]:
            io_obj = self.make_new_io_counter3k[mca.cmd["name"]]()
            
        io_obj.adc_sr = mca.adc_sr
        
        if ('num_items' not in mca.cmd) or ('num_items' in mca.cmd and mca.cmd['num_items'] == 0):
                mca.cmd['num_items'] = io_obj.num_items
        
        mca.mem_type = 0  # RAM by default
        if 'memory' in mca.cmd and mca.cmd['memory']=='flash':
            mca.mem_type = 1
        if 'memory' in mca.cmd and mca.cmd['memory']=='reset':
            mca.mem_type = 2 
            
        if mca.cmd['dir'] == 'read':  # It is a read command
            self.data_read(mca, io_obj)
            return None
            
        call = {
            "write": self.data_write, "short_write": self.short_write, "rmw": self.read_modify_write,
            "update": self.read_modify_write, "fields_to_user": self.fields_to_user, "user_to_fields": self.user_to_fields
        }
        call[mca.cmd['dir']](mca, io_obj)

    def short_write(self, mca, io_obj):
        """
            Input is mca.data_out_list which is written to the target.  This is not a read-modify-write command.
            This only covers short writes where the command and data are transmitted to the command end point,
            typically writing a list of FPGA CTRL or ACTION registers.
        """
        # Replace the first entry in a fixed-length 16-word uint32 list
        
        num_items = mca.cmd['num_items']
        num_items = min(num_items, 60//item_size_dict[io_obj.data_type])
        num_bytes = num_items * item_size_dict[io_obj.data_type]
        header = (num_bytes << 16) + (mca.mem_type << 12) + 0x800 + (io_obj.cmd_addr << 4) + io_obj.wr_type

        # Create command-out byte array and send
        mca.write_ep = mca.cmd_out_ep
        struct.pack_into("<1I", mca.bytes_out, 0, header)  # 32-bit header comes first
        # Add data after the header
        struct.pack_into("<{}{}".format(num_items, io_obj.data_type),
                         mca.bytes_out, *mca.data_out_list[0:num_items])
#         if io_obj.data_type == 'H':
#             struct.pack_into("<{}H", mca.bytes_out, 4, *mca.data_out_list[0:num_items])
#         elif io_obj.data_type == 'I':
#             struct.pack_into("<{}I", mca.bytes_out, 4, *mca.data_out_list[0:num_items])
#         elif io_obj.data_type == 'f':
#             struct.pack_into("<{}f", mca.bytes_out, 4, *mca.data_out_list[0:num_items])
        mca.num_bytes = num_bytes + 4
        mca.write_data()  # Write to the cmd_out endpoint

    def write_command(self, mca, io_obj, com_type):
        num_bytes = int(mca.cmd['num_items']*item_size_dict[io_obj.data_type])
        #num_bytes = max(64, num_bytes)  # Force requesting at least 64 bytes
        header = (num_bytes << 16) + (mca.mem_type << 12) + (io_obj.cmd_addr << 4) + com_type
        io_obj.add_to_cmd_out_list(mca)  # Uses cmd_ctrl dict values, if any, to make additions to mca.cmd_out_list .

        # Create command-out byte array
        mca.write_ep = mca.cmd_out_ep
        struct.pack_into("<1I", mca.bytes_out, 0, header)  # cmd_out data, to be sent to the device
        struct.pack_into("<30H", mca.bytes_out, 4, *mca.cmd_out_list[0: 30])  # cmd_out data, to be sent to the device
        mca.num_bytes = 64
        mca.write_data()  # Write to the command endpoint
           
    def data_read(self, mca, io_obj):
        """
            Read data from the mca.
            Results are posted as a dictionary in mca.data_in_dict
        """
        
        self.write_command(mca, io_obj, io_obj.rd_type)

        # Now read the data and convert to higher level data
        mca.read_ep = mca.data_in_ep
        mca.num_bytes = mca.cmd['num_items']*item_size_dict[io_obj.data_type]
        mca.read_data()  # Read the data, fills mca.bytes_in
        mca.data_in_list = struct.unpack_from("<{}{}".format(mca.cmd['num_items'], io_obj.data_type), mca.bytes_in)
        io_obj.registers = mca.data_in_list
        io_obj.registers_2_fields()
        io_obj.fields_2_user()
        mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
       
        
    def data_write(self, mca, io_obj):
        """ Input is mca.data_out_list which is written to the target.  This is not a read-modify-write command
            This does not cover short writes where the data are transmitted to the command end point.
            It should be used for special cases to save some time compared with the recommended read-modify-write.
        """
        self.write_command(mca, io_obj, io_obj.wr_type)
        
        # Pack data-out byte array and send
        mca.write_ep = mca.data_out_ep
        mca.num_bytes = mca.cmd['num_items']*item_size_dict[io_obj.data_type]
        mca.bytes_out = array.array('B', [0] * max(64, mca.num_bytes))  # Create a send byte buffer
        struct.pack_into("<{}{}".format(mca.cmd["num_items"], io_obj.data_type), mca.bytes_out, 0, *mca.data_out_list)
        mca.write_data()  # Write to the data_out endpoint
        if mca.mem_type == 1:
            time.sleep(0.1)  # To give the ARM time to complete the write to the flash memory.

        
    def read_modify_write(self, mca, io_obj):
        """Input is mca.data_out_dict.  That dictionary contains the values that should be updated inside the device.
           This is a read-modify-write command:
           Read data from device and create a dictionary,
           Update that dictionary with the user-supplied values,
           Write the data back to the device.
        """
        self.data_read(mca, io_obj)

        # Update the dictionary with the user-supplied data and convert back to an updated list

        if "fields" in mca.data_out_dict:
            io_obj.fields.update(mca.data_out_dict["fields"])
            io_obj.fields_2_user()
            io_obj.fields_2_registers()
        if "user" in mca.data_out_dict:
            io_obj.user.update(mca.data_out_dict["user"])
            io_obj.user_2_fields()
            io_obj.fields_2_registers()

        io_obj.user_2_fields()
        io_obj.fields_2_registers()
        mca.data_out_list = io_obj.registers
        mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}

        self.data_write(mca, io_obj)
        return None
        
        
    def align(self, mca, io_obj):
        """Input is mca.data_out_dict.  That dictionary contains the values that need to be synchronized
           This command does not communicate with an MCA
           It only makes sure that user, fields and registers do not contradict each other.
        """
        self.data_read(mca, io_obj)
        if "fields" in mca.data_out_dict:
            io_obj.fields.update(mca.data_out_dict["fields"])
            io_obj.fields_2_user()
            io_obj.fields_2_registers()
        if "user" in mca.data_out_dict:
            io_obj.user.update(mca.data_out_dict["user"])
            io_obj.user_2_fields()
            io_obj.fields_2_registers()

        io_obj.user_2_fields()
        io_obj.fields_2_registers()
        mca.data_out_list = io_obj.registers
        mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
        return None
        
    def fields_to_user(self, mca, io_obj):
        """
            Input is mca.data_out_dict.  
            If the user changed a 'fields' value, they can ensure that any corresponding 'user' value will be updated.
            This command does not communicate with an MCA
            It only makes sure that user, fields and registers do not contradict each other.
        """
        self.data_read(mca, io_obj)
        if "fields" in mca.data_out_dict:
            io_obj.fields.update(mca.data_out_dict["fields"])
            io_obj.fields_2_user()
            io_obj.fields_2_registers()

        mca.data_out_list = io_obj.registers
        mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
        return None

    def user_to_fields(self, mca, io_obj):
        """
            Input is mca.data_out_dict.  
            If the user changed a 'user' value, they can ensure that any corresponding 'fields' value will be updated.
            This command does not communicate with an MCA
            It only makes sure that user, fields and registers do not contradict each other.
        """
        self.data_read(mca, io_obj)
        if "user" in mca.data_out_dict:
            io_obj.user.update(mca.data_out_dict["user"])
            io_obj.user_2_fields()
            io_obj.fields_2_registers()

        mca.data_out_list = io_obj.registers
        mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
        return None
        

# class ftdi_api():
#     """
#         Communication class to provide support for eMorpho devices, which do not have an embedded ARM processor.
#     """
#     def __init__(self):
#         # List of command targets
#         # Objects will support read, short_write, write, rmw as appropriate
#         # Objects will support cmd['memory']='flash' as appropriate.
#         self.make_new_io_obj = {
#             'arm_ping': emorpho_data.arm_ping,
#             'fpga_ctrl': emorpho_data.fpga_ctrl,
#             'fpga_statistics': emorpho_data.fpga_statistics,
#             'fpga_results': emorpho_data.fpga_results,
#             'fpga_histogram': emorpho_data.fpga_histogram,
#             'fpga_list_mode': emorpho_data.fpga_list_mode,
#             'fpga_trace': emorpho_data.fpga_trace,
#             'fpga_user': emorpho_data.fpga_user,
#             'fpga_weights': emorpho_data.fpga_weights,
#             'fpga_time_slice': emorpho_data.fpga_time_slice,
#             'fpga_lm_2b': emorpho_data.fpga_lm_2b,
#         }
# 
#         
#     def perform_cmd(self, mca):
#         """
#         Create a local io_obj according to the command name and execute the command.
#         :param mca: An mca object describes one eMorpho
#         :return: None
#         """
#         io_obj = self.make_new_io_obj[mca.cmd["name"]]()
#         io_obj.adc_sr = mca.adc_sr
# 
#         if mca.cmd['memory'] == "ram" and mca.cmd['dir'] == "read":
#             self.read_ram(mca, io_obj)
#             return None
# 
#         if mca.cmd['memory'] == "flash" and mca.cmd['dir'] == "read":
#             self.read_flash(mca)
#             return None
# 
#         if mca.cmd['dir'] == 'short_write':  # It is a write command where exactly 32-bytes of data are transmitted
#             """Input is mca.data_out_list which is written to the target.  This is not a read-modify-write command
#             """
#             self.short_write(mca)
#             return None
# 
#         if mca.cmd['memory'] == "ram" and mca.cmd['dir'] == "write":
#             self.write_ram(mca, io_obj)
#             return None
# 
#         if mca.cmd['memory'] == "flash" and mca.cmd['dir'] == "write":
#             self.write_flash(mca)
#             return None
# 
#         if mca.cmd['dir'] == 'rmw':  # It is a read-modify-write command
#             self.read_modify_write_ram(mca, io_obj)
#             return None
#             
#         if mca.cmd['dir'] == 'fields_to_user':  # Only update user with fields as an input
#             self.fields_to_user(mca, io_obj)
#             return None
#             
#         if mca.cmd['dir'] == 'user_to_fields':  # Only update fields with user as an input
#             self.user_to_fields(mca, io_obj)
#             return None
# 
# 
#     def read_ram(self, mca, io_obj):
#         offset = 0
#         if "offset" in mca.cmd:
#             offset = mca.cmd["offset"]
#         # Host omits this, or sets this to zero to get default value for num_items
#         if 'num_items' not in mca.cmd or mca.cmd['num_items'] == 0:
#             mca.cmd['num_items'] = io_obj.num_items
#         bpi = {"H": 2, "I": 4}
#         off = int(offset*bpi[io_obj.data_type]/512)
#         # Make a fixed-length 2-byte header
#         mca.num_bytes = mca.cmd['num_items'] * item_size_dict[io_obj.data_type]
#         mca.cmd_out_list = [(io_obj.cmd_addr << 8) + (off << 3) + 0x7]  # Packet header for a short write
# 
#         # Short write to program the read address into the FPGA
#         mca.bytes_out = array.array('B', [0] * 2)
#         struct.pack_into("<1H", mca.bytes_out, 0, *mca.cmd_out_list)  # cmd_out data, to be sent to the device
#         mca.data_out_list = []  # No other data to follow
#         mca.write_data()  # Write to the command endpoint
# 
#         # Now read the data and convert to higher level data
#         # Because of the status bytes in the byte stream it is much simpler to unpack inside mca.read_data()
#         mca.read_data(2 if io_obj.data_type == 'H' else 4)  # Read the data, fills mca.bytes_in
#         # mca.data_in_list = struct.unpack_from("<{}{}".format(mca.cmd['num_items'], io_obj.data_type), mca.bytes_in)
#         io_obj.registers = mca.data_in_list
#         io_obj.registers_2_fields()
#         io_obj.fields_2_user()
# 
#         mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
#         return None
# 
# 
#     def write_ram(self, mca, io_obj):
#         """
#             Input is mca.data_out_list which is written to the target. The function writes complete chunks
#             of 16 uint16_t words.  Incomplete chunks will not be written; eg if there are 20 data in the list,
#             only 16 will be written to the device.
#             This is not a read-modify-write command
#         """
#         num_chunks = len(mca.data_out_list) // 16  # Expecting n*16 uint16_t data in mca.data_out_list
#         mca.num_bytes = 34
#         num_words = mca.num_bytes // 2
#         mca.cmd_out_list = [0] * num_words  # 2-byte packet header + 16 uint16_t values
#         mca.bytes_out = array.array('B', [0] * mca.num_bytes)
#         for n in range(num_chunks):
#             # Make the packet header for the first write buffer
#             if n == 0:
#                 mca.cmd_out_list[0] = (io_obj.cmd_addr << 8) + 0x1
#             else:
#                 mca.cmd_out_list[0] = io_obj.cmd_addr << 8
#             mca.cmd_out_list[1:num_words] = mca.data_out_list[n * (num_words - 1):(n + 1) * (num_words - 1)]
#             struct.pack_into("<{}H".format(num_words), mca.bytes_out, 0, *mca.cmd_out_list)
#             mca.write_data()  # Write to the data_out endpoint
# 
#         return None
# 
# 
#     def short_write(self, mca, io_obj):
#         # Make the packet header for the first write buffer
#         mca.num_bytes = 34
#         num_words = mca.num_bytes // 2
#         mca.cmd_out_list = [0] * num_words  # 2-byte packet header + 16 uint16_t values
#         mca.cmd_out_list[0] = (io_obj.cmd_addr << 8) + 0x3
#         mca.cmd_out_list[1:num_words] = mca.data_out_list[0:num_words - 1]
# 
#         # Create data-out byte array and send
#         mca.bytes_out = array.array('B', [0] * mca.num_bytes)
#         # data_out data, to be sent to the device
#         struct.pack_into("<{}H".format(num_words), mca.bytes_out, 0, *mca.cmd_out_list)
#         mca.write_data()  # Write to the data_out endpoint
#         return None
# 
# 
#     def read_flash(self, mca):
#         # Copy nv-mem content into user area
#         ctrl_obj = self.make_new_io_obj["fpga_ctrl"]()
#         ctrl_obj.adc_sr = mca.adc_sr
#         mca.data_out_dict["fields"] = {"read_nv": 1}
#         self.read_modify_write_ram(mca, ctrl_obj)
#         time.sleep(0.02)
# 
#         # read data from the user area
#         mca.cmd.update({"num_items": 64})
#         user_obj = self.make_new_io_obj["fpga_user"]()
#         self.read_ram(mca, user_obj)
# 
#         # Convert register data to 'fields' and 'user'
#         nv_mem_valid = mca.data_in_list[0] == 0x8003
#         if nv_mem_valid:
#             ctrl_obj.registers = mca.data_in_list[1:17]
#             ctrl_obj.registers_2_fields()
#             ctrl_obj.fields_2_user()
#             mca.data_in_dict = {"registers": mca.data_in_list[1:17], "fields": ctrl_obj.fields, "user": ctrl_obj.user}
#         else:
#             mca.data_in_dict = {"registers": mca.data_in_list[0:17]}
# 
#         return None
# 
# 
#     def write_flash(self, mca):
#         ctrl_obj = self.make_new_io_obj["fpga_ctrl"]()
#         ctrl_obj.adc_sr = mca.adc_sr
#         self.read_ram(mca, ctrl_obj)
# 
#         user_obj = self.make_new_io_obj["fpga_user"]()
#         mca.data_out_list = [0x8003] + ctrl_obj.registers + [0]*15  # List length must be a multiple of 16
#         self.write_ram(mca, user_obj)
# 
#         ctrl_obj.fields.update({"write_nv": 1})
#         ctrl_obj.fields_2_registers()
# 
#         mca.data_out_list = ctrl_obj.registers
#         self.write_ram(mca, ctrl_obj)
#         time.sleep(0.02)
# 
#         return None
# 
# 
#     def read_modify_write_ram(self, mca, io_obj):
#         """Input is mca.data_out_dict.  That dictionary contains the values that should be  updated inside the device.
#            This is a read-modify-write command:
#            Read data from device and create a dictionary,
#            Update that dictionary with the user-supplied values,
#            Write the data back to the device.
#         """
# 
#         mca.cmd['num_items'] = io_obj.num_items  # Use the default number to ensure correct conversion to 'fields'
# 
#         # Make a fixed-length 2-byte header
#         mca.num_bytes = mca.cmd['num_items'] * item_size_dict[io_obj.data_type]
#         mca.cmd_out_list = [(io_obj.cmd_addr << 8) + (io_obj.offset << 3) + 0x7]  # Packet header for a short write
# 
#         # Short write to program the read address into the FPGA
#         mca.bytes_out = array.array('B', [0] * 2)
#         struct.pack_into("<1H", mca.bytes_out, 0, *mca.cmd_out_list)  # cmd_out data, to be sent to the device
#         mca.data_out_list = []  # No other data to follow
#         mca.write_data()  # Write to the command endpoint
# 
#         # Now read the data and convert to higher level data
#         mca.read_data(2 if io_obj.data_type == 'H' else 4)  # Read the data, fills mca.bytes_in
#         # mca.data_in_list = struct.unpack_from("<{}{}".format(mca.cmd['num_items'], io_obj.data_type), mca.bytes_in)
#         io_obj.registers = mca.data_in_list
#         io_obj.registers_2_fields()
#         io_obj.fields_2_user()
# 
#         # Update the dictionary with the user-supplied data and convert back to an updated list
#         if "fields" in mca.data_out_dict:
#             io_obj.fields.update(mca.data_out_dict["fields"])
#             io_obj.fields_2_user()
#             io_obj.fields_2_registers()
#         if "user" in mca.data_out_dict:
#             io_obj.user.update(mca.data_out_dict["user"])
#             io_obj.user_2_fields()
#             io_obj.fields_2_registers()
# 
#         io_obj.user_2_fields()
#         io_obj.fields_2_registers()
#         mca.data_out_list = io_obj.registers
#         
#         # Write data back to the device
# 
#         num_chunks = len(mca.data_out_list) // 16  # Expecting n*16 uint16_tdata in mca.data_out_list
#         mca.num_bytes = 34
#         num_words = mca.num_bytes // 2
#         mca.cmd_out_list = [0] * num_words  # 2-byte packet header + 16 uint16_t values
#         mca.bytes_out = array.array('B', [0] * mca.num_bytes)
#         for n in range(num_chunks):
#             # Make the packet header for the first write buffer
#             if n == 0:
#                 mca.cmd_out_list[0] = (io_obj.cmd_addr << 8) + 0x3
#             else:
#                 mca.cmd_out_list[0] = io_obj.cmd_addr << 8
#             mca.cmd_out_list[1:num_words] = mca.data_out_list[n * (num_words - 1):(n + 1) * (num_words - 1)]
#             struct.pack_into("<{}H".format(num_words), mca.bytes_out, 0, *mca.cmd_out_list)
#             mca.write_data()  # Write to the data_out endpoint
# 
#         # Report back the state of the eMorpho FPGA controls
#         mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
#         
#         return None
# 
#     def fields_to_user(self, mca, io_obj):
#         """
#             Input is mca.data_out_dict.  
#             If the user changed a 'fields' value, they can ensure that any corresponding 'user' value will be updated.
#             This command does not communicate with an MCA
#             It only makes sure that user, fields and registers do not contradict each other.
#         """
#         self.read_ram(mca, io_obj)
#         if "fields" in mca.data_out_dict:
#             io_obj.fields.update(mca.data_out_dict["fields"])
#             io_obj.fields_2_user()
#             io_obj.fields_2_registers()
# 
#         mca.data_out_list = io_obj.registers
#         mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}
# 
#     def user_to_fields(self, mca, io_obj):
#         """
#             Input is mca.data_out_dict.  
#             If the user changed a 'user' value, they can ensure that any corresponding 'fields' value will be updated.
#             This command does not communicate with an MCA
#             It only makes sure that user, fields and registers do not contradict each other.
#         """
#         self.read_ram(mca, io_obj)
#         if "user" in mca.data_out_dict:
#             io_obj.user.update(mca.data_out_dict["user"])
#             io_obj.user_2_fields()
#             io_obj.fields_2_registers()
# 
#         mca.data_out_list = io_obj.registers
#         mca.data_in_dict = {'registers': io_obj.registers, 'fields': io_obj.fields, 'user': io_obj.user}

