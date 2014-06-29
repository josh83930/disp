import os
import ctypes
from ctypes import c_uint, c_uint32, c_uint16
from record_info import *

cwd = os.path.dirname(os.path.realpath(__file__))
libconthost = ctypes.cdll.LoadLibrary(os.path.join(os.path.dirname(cwd),'lib/libconthost.so'))

class Dispatch(object):
    """Receive data from a dispatch stream."""
    def __init__(self, host):
        """Connect to the dispatcher at hostname `host`."""
        if not isinstance(host,bytes):
            # python 3 strings are not char arrays!
            host = bytes(host,'ascii')

        self.rc = libconthost.init_disp_link(host, b'w RAWDATA w RECHDR')

        if self.rc > 0:
            libconthost.send_me_always()
            libconthost.my_id(b"QDISPATCH")
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
        len = self._recv(data_ptr, block)

        if len is None:
            return None

        header = ctypes.cast(data_ptr,ctypes.POINTER(GenericRecordHeader)).contents

        # pointer just past the header
        o = ctypes.byref(data,ctypes.sizeof(GenericRecordHeader))

        if header.RecordID == records['PMT_RECORD']:
            # cast to PmtEventRecord struct
            event_record = ctypes.cast(o,ctypes.POINTER(PmtEventRecord)).contents
        elif header.RecordID in records.values():
            raise NotImplementedError("Unable to decode record type")
        else:
            raise Exception("Unknown record type")

    def _recv(self, data, block=True):
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
    d = Dispatch('127.0.0.1')
    d.next()
