import os
import ctypes
from ctypes import c_uint, c_uint32, c_uint16

cwd = os.path.dirname(os.path.realpath(__file__))
libconthost = ctypes.cdll.LoadLibrary(os.path.join(os.path.dirname(cwd),'lib/libconthost.so'))

class GenericRecordHeader(ctypes.BigEndianStructure):
    _fields_ = [('record_id',      ctypes.c_uint32),
                ('record_length',  ctypes.c_uint32),
                ('record_version', ctypes.c_uint32)]


#   Master Trigger Card data 
class MTCReadoutData(ctypes.BigEndianStructure):
    _fields_ = [ #   word 0 
                ('Bc10_1',            c_uint32,  32),
                #   word 1 
                ('Bc50_1',            c_uint32,  11),
                ('Bc10_2',            c_uint32,  21),
                #   word 2 
                ('Bc50_2',            c_uint32,  32),
                #   word 3 
                ('Owln',              c_uint,    1), #   MSB 
                ('ESum_Hi',           c_uint,    1),
                ('ESum_Lo',           c_uint,    1),
                ('Nhit_20_LB',        c_uint,    1),
                ('Nhit_20',           c_uint,    1),
                ('Nhit_100_Hi',       c_uint,    1),
                ('Nhit_100_Med',      c_uint,    1),
                ('Nhit_100_Lo',       c_uint,    1),
                ('BcGT',              c_uint32,  24), #   LSB 
                #   word 4 
                ('Diff_1',            c_uint,    3),
                ('Peak',              c_uint,    10),
                ('Miss_Trig',         c_uint,    1),
                ('Soft_GT',           c_uint,    1),
                ('NCD_Mux',           c_uint,    1),
                ('Special_Raw',       c_uint,    1),
                ('Ext_8',             c_uint,    1),
                ('NCD_Shaper',        c_uint,    1),
                ('Ext_6',             c_uint,    1),
                ('Ext_5',             c_uint,    1),
                ('Ext_4',             c_uint,    1),
                ('Ext_3',             c_uint,    1),
                ('Hydrophone',        c_uint,    1),
                ('Ext_Async',         c_uint,    1),
                ('Sync',              c_uint,    1),
                ('Pong',              c_uint,    1),
                ('Pedestal',          c_uint,    1),
                ('Prescale',          c_uint,    1),
                ('Pulse_GT',          c_uint,    1),
                ('Owle_Hi',           c_uint,    1),
                ('Owle_Lo',           c_uint,    1),
                #  word 5 
                ('Unused3',           c_uint,    1),
                ('Unused2',           c_uint,    1),
                ('Unused1',           c_uint,    1),
                ('FIFOsAllFull',      c_uint,    1),
                ('FIFOsNotAllFull',   c_uint,    1),
                ('FIFOsNotAllEmpty',  c_uint,    1),
                ('SynClr24_wo_TC24',  c_uint,    1),
                ('SynClr24',          c_uint,    1),
                ('SynClr16_wo_TC16',  c_uint,    1),
                ('SynClr16',          c_uint,    1),
                ('TestMem2',          c_uint,    1),
                ('TestMem1',          c_uint,    1),
                ('Test10',            c_uint,    1),
                ('Test50',            c_uint,    1),
                ('TestGT',            c_uint,    1),
                ('Int',               c_uint,    10),
                ('Diff_2',            c_uint,    7)]

class PmtEventRecord(ctypes.BigEndianStructure):
    _fields_ = [('PmtEventRecord',  c_uint16),
                ('DataType',        c_uint16),
                ('RunNumber',       c_uint32),
                ('EvNumber',        c_uint32),
                ('DaqStatus',       c_uint16),
                ('NPmtHit',         c_uint16),
                ('CalPckType',      c_uint32), # used to store sub-run number
                ('TriggerCardData', MTCReadoutData)]

TAGSIZE = 8
BUFFER_SIZE = 130000

class Dispatch(object):
    def __init__(self, host):
        self.rc = libconthost.init_disp_link(host, b'w RAWDATA w RECHDR')

        if self.rc > 0:
            libconthost.send_me_always()
            libconthost.my_id(b"QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    def next(self, block=True):
        data = ctypes.create_string_buffer(BUFFER_SIZE)
        data_ptr = ctypes.byref(data)
        len = self.recv(data_ptr, block)

        if len is None:
            return None

        header = ctypes.cast(data_ptr,ctypes.POINTER(GenericRecordHeader)).contents

        # pointer just past the header
        o = ctypes.byref(data,ctypes.sizeof(GenericRecordHeader))

        if header.RecordID == PMT_RECORD:
            # cast to PmtEventRecord struct
            event_record = ctypes.cast(o,ctypes.POINTER(PmtEventRecord)).contents
        else:
            raise Exception("Unknown record type")

    def recv(self, data, block=True):
        nbytes = ctypes.c_int()
        dtag = ctypes.create_string_buffer(TAGSIZE+1)
        ctypes.memset(ctypes.byref(dtag),0,TAGSIZE+1)

        if block:
            rc = libconthost.wait_head(dtag, ctypes.byref(nbytes))
        else:
            rc = libconthost.check_head(dtag, ctypes.byref(nbytes))
            if not rc:
                return None

        if rc > 0:
            if nbytes.value < BUFFER_SIZE:
                rc = libconthost.get_data(data, nbytes)
            else:
                raise Exception("Insufficient buffer size")

        return nbytes.value


    def __del__(self):
        libconthost.drop_connection()

if __name__ == '__main__':
    d = Dispatch(b'127.0.0.1')
    d.next()
