import os
import ctypes

cwd = os.path.dirname(os.path.realpath(__file__))
libconthost = ctypes.cdll.LoadLibrary(os.path.join(os.path.dirname(cwd),'lib/libconthost.so'))

class GenericRecordHeader(ctypes.BigEndianStructure):
    _fields_ = [('record_id',      ctypes.c_uint32),
                ('record_length',  ctypes.c_uint32),
                ('record_version', ctypes.c_uint32)]

TAGSIZE = 8
BUFFER_SIZE = 130000

class Dispatch(object):
    def __init__(self, host):
        self.rc = libconthost.init_disp_link(host, 'w RAWDATA w RECHDR')

        if self.rc > 0:
            libconthost.send_me_always()
            libconthost.my_id("QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    def next(self, block=True):
        data = ctypes.create_string_buffer(BUFFER_SIZE)
        data_ptr = ctypes.byref(data)
        len = self.recv(data_ptr, block)

        if len is None:
            return None

        header = ctypes.cast(data_ptr,ctypes.POINTER(GenericRecordHeader)).contents

        print header.record_id
        print header.record_length
        print header.record_version

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
    d = Dispatch('127.0.0.1')
    d.next()
