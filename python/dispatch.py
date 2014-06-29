import os
import ctypes

cwd = os.path.dirname(os.path.realpath(__file__))
libconthost = ctypes.cdll.LoadLibrary(os.path.join(os.path.dirname(cwd),'lib/libconthost.so'))

class Dispatch(object):
    def __init__(self, host):
        self.rc = libconthost.init_disp_link(host, 'w RAWDATA w RECHDR')

        if self.rc > 0:
            libconthost.send_me_always()
            libconthost.my_id("QDISPATCH")
        else:
            raise Exception("Could not connect to dispatch at %s" % host)

    def __del__(self):
        libconthost.drop_connection()
