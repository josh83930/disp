import os
import ctypes
from record_info import *
import numpy as np

cwd = os.path.dirname(os.path.realpath(__file__))

def iter_pmt_hits(pev):
    """
    Iterate over PMT hits (FECReadoutData structures). The argument `pev` should be
    a PMTEventRecord structure.
    """
    p = ctypes.byref(pev,ctypes.sizeof(PmtEventRecord))
    for pmt in ctypes.cast(p,ctypes.POINTER(FECReadoutData*pev.NPmtHit)).contents:
        yield pmt

def get_trigger_type(pev):
    """Returns the trigger type from a PmtEventRecord."""
    mtc_ptr = ctypes.byref(pev.TriggerCardData)
    mtc_words = ctypes.cast(mtc_ptr,ctypes.POINTER(ctypes.c_uint32*6)).contents
    mtc_words = np.frombuffer(mtc_words, dtype=np.uint32, count=6).byteswap()
    return ((mtc_words[3] & 0xff000000) >> 24) | ((mtc_words[4] & 0x3ffff) << 8)

class Dispatch(object):
    """Receive data from a dispatch stream."""
    def __init__(self, host):
        """Connect to the dispatcher at hostname `host`."""
        self.libconthost = ctypes.cdll.LoadLibrary(os.path.join(os.path.dirname(cwd),'lib/libconthost.so'))
        if not isinstance(host,bytes):
            # python 3 strings are not char arrays!
            host = bytes(host,'ascii')

        self.rc = self.libconthost.init_disp_link(host, b'w RAWDATA w RECHDR')

        if self.rc > 0:
            self.libconthost.send_me_always()
            self.libconthost.my_id(b"QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    def next(self, block=True):
        """
        Returns the next event from the dispatch stream. If `block` is True,
        block until an event is received. If `block` is False, and no event
        is available, returns None.
        """
        data = ctypes.create_string_buffer(BUFFER_SIZE)
        data_ptr = ctypes.byref(data)
        nbytes = self._recv(data_ptr, block)

        if nbytes is None:
            return None

        header = ctypes.cast(data_ptr,ctypes.POINTER(GenericRecordHeader)).contents

        # pointer just past the header
        o = ctypes.byref(data,ctypes.sizeof(GenericRecordHeader))

        if header.RecordID == records['PMT_RECORD']:
            # cast to PmtEventRecord struct
            event_record = ctypes.cast(o,ctypes.POINTER(PmtEventRecord)).contents
            # attach the data buffer so it doesn't get garbage collected
            event_record.data = data
            return event_record
        elif header.RecordID in records.values():
            raise NotImplementedError("Unable to decode record type")
        else:
            raise Exception("Unknown record type")

    def _recv(self, data, block=True):
        nbytes = ctypes.c_int()
        dtag = ctypes.create_string_buffer(TAGSIZE+1)
        ctypes.memset(ctypes.byref(dtag),0,TAGSIZE+1)

        if block:
            rc = self.libconthost.wait_head(dtag, ctypes.byref(nbytes))
        else:
            rc = self.libconthost.check_head(dtag, ctypes.byref(nbytes))
            if not rc:
                return None

        if rc > 0:
            if nbytes.value < BUFFER_SIZE:
                rc = self.libconthost.get_data(data, nbytes)
            else:
                raise Exception("Insufficient buffer size")

        return nbytes.value


    def __del__(self):
        self.libconthost.drop_connection()

