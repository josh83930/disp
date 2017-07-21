import ctypes
import struct
from record_info import GenericRecordHeader, FECReadoutData, RECORD_IDS, PmtEventRecord

_TAGSIZE = 8
_MAX_RECORD_SIZE = 130000

def unpack_header(data):
    """
    Unpacks the record header from `data` and returns the record type
    and the rest of the record data as a tuple.
    """
    header = GenericRecordHeader.from_buffer_copy(data)

    return header.RecordID, data[ctypes.sizeof(GenericRecordHeader):]

def unpack_pmt_record(data):
    """
    Unpacks a PMT event record. This method returns a generator which first
    yields the PmtEventRecord struct and then the individual PMT records
    (FECReadoutData structs).
    """
    fec_data_size = ctypes.sizeof(FECReadoutData)
    pmt_event_size = ctypes.sizeof(PmtEventRecord)

    event_record = PmtEventRecord.from_buffer_copy(data)
    yield event_record

    for i in range(0, event_record.NPmtHit):
        offset = pmt_event_size + i*fec_data_size
        yield FECReadoutData.from_buffer_copy(data, offset)

def unpack_trigger_type(pev):
    """Returns the trigger type from a PmtEventRecord."""
    mtc_words = struct.unpack('>IIIIII', pev.TriggerCardData)

    return ((mtc_words[3] & 0xff000000) >> 24) | ((mtc_words[4] & 0x7ffff) << 8)

class Dispatch(object):
    """Receive data from a dispatch stream."""
    def __init__(self, host):
        """Connect to the dispatcher at hostname `host`."""
        self.libconthost = ctypes.cdll.LoadLibrary('libconthost.so')

        if not isinstance(host,bytes):
            # python 3 strings are not char arrays!
            host = bytes(host,'ascii')

        self.rc = self.libconthost.init_disp_link(host, b'w RAWDATA w RECHDR')

        if self.rc > 0:
            self.libconthost.send_me_always()
            self.libconthost.my_id(b"QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    def recv(self, block=True):
        """Returns the next record from the dispatcher as a string buffer."""
        nbytes = ctypes.c_int()
        dtag = ctypes.create_string_buffer(_TAGSIZE+1)
        ctypes.memset(ctypes.byref(dtag),0,_TAGSIZE+1)

        if block:
            rc = self.libconthost.wait_head(dtag, ctypes.byref(nbytes))
        else:
            rc = self.libconthost.check_head(dtag, ctypes.byref(nbytes))
            if not rc:
                return None

        if nbytes.value > _MAX_RECORD_SIZE:
            raise BufferError("Insufficient buffer size")

        data = ctypes.create_string_buffer(nbytes.value)

        if rc > 0:
            rc = self.libconthost.get_data(data, nbytes)
        else:
            raise RuntimeError('check_head() or wait_head() returned %i' % rc)

        return data

    def __iter__(self):
        while True:
            yield self.recv()

    def __del__(self):
        self.libconthost.drop_connection()

