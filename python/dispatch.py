import os
import ctypes
from ctypes import sizeof, byref, cast, POINTER
from record_info import *
import numpy as np
import sys

_cwd = os.path.dirname(os.path.realpath(__file__))
_libconthost_path = os.path.join(os.path.dirname(_cwd),'lib/libconthost.so')

def iter_pmt_hits(pev):
    """
    Iterate over PMT hits (FECReadoutData structures). The argument `pev` should be
    a PMTEventRecord structure.
    """
    p = byref(pev,sizeof(PmtEventRecord))
    for pmt in cast(p,POINTER(FECReadoutData*pev.NPmtHit)).contents:
        yield pmt

def get_trigger_type(pev):
    """Returns the trigger type from a PmtEventRecord."""
    mtc_ptr = byref(pev.TriggerCardData)
    mtc_words = cast(mtc_ptr,POINTER(ctypes.c_uint32*6)).contents
    mtc_words = np.frombuffer(mtc_words, dtype=np.uint32, count=6)
    if sys.byteorder == 'little':
        mtc_words = mtc_words.byteswap()
    return ((mtc_words[3] & 0xff000000) >> 24) | ((mtc_words[4] & 0x3ffff) << 8)

class Dispatch(object):
    """Receive data from a dispatch stream."""
    def __init__(self, host):
        """Connect to the dispatcher at hostname `host`."""
        self.libconthost = ctypes.cdll.LoadLibrary(_libconthost_path)
        if not isinstance(host,bytes):
            # python 3 strings are not char arrays!
            host = bytes(host,'ascii')

        self.rc = self.libconthost.init_disp_link(host, b'w RAWDATA w RECHDR')

        if self.rc > 0:
            self.libconthost.send_me_always()
            self.libconthost.my_id(b"QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    @profile
    def next(self, block=True):
        """
        Returns the next event from the dispatch stream. If `block` is True,
        block until an event is received. If `block` is False, and no event
        is available, returns None.
        """
        data = ctypes.create_string_buffer(BUFFER_SIZE)
        data_ptr = byref(data)
        nbytes = self._recv(data_ptr, block)

        if nbytes is None:
            return None

        header = cast(data_ptr,POINTER(GenericRecordHeader)).contents

        # pointer just past the header
        o = byref(data,sizeof(GenericRecordHeader))

        if header.RecordID == records['PMT_RECORD']:
            # cast to PmtEventRecord struct
            event_record = cast(o,POINTER(PmtEventRecord)).contents
            # attach the data buffer so it doesn't get garbage collected
            event_record.data = data
            return event_record
        elif header.RecordID in records.values():
            raise NotImplementedError("Unable to decode record type")
        else:
            raise TypeError("Unknown record type")

    @profile
    def _recv(self, data, block=True):
        nbytes = ctypes.c_int()
        dtag = ctypes.create_string_buffer(TAGSIZE+1)
        ctypes.memset(byref(dtag),0,TAGSIZE+1)

        if block:
            rc = self.libconthost.wait_head(dtag, byref(nbytes))
        else:
            rc = self.libconthost.check_head(dtag, byref(nbytes))
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

