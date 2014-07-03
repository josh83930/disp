import os
import ctypes
from ctypes import sizeof, byref, cast, POINTER
from record_info import *
import numpy as np
import sys

_cwd = os.path.dirname(os.path.realpath(__file__))
_libconthost_path = os.path.join(os.path.dirname(_cwd),'lib/libconthost.so')

def unpack_header(data):
    """
    Unpacks the record header from `data` and returns the record type
    and the record data as a tuple.
    """
    header = GenericRecordHeader.from_buffer_copy(data)

    if header.RecordID not in records.values():
        raise TypeError('Unknown record id {id}'.format(header.RecordID))

    return header.RecordID, data[sizeof(GenericRecordHeader):]

def unpack_pmt_record(data):
    """
    Unpacks a PMT event record. This method returns a generator which first
    yields the PmtEventRecord struct and then the individual PMT records
    (FECReadoutData structs).
    """
    event_record = PmtEventRecord.from_buffer_copy(data)

    yield event_record

    for i in range(0,event_record.NPmtHit,sizeof(FECReadoutData)):
        yield FECReadoutData.from_buffer_copy(data,sizeof(PmtEventRecord)+i)

def unpack_trigger_type(pev):
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

    def next(self, block=True):
        """
        Returns the next event from the dispatch stream. If `block` is True,
        block until an event is received. If `block` is False, and no event
        is available, returns None.
        """
        data = self.recv(block)

        if data is None:
            # no data ready
            return None

        header = GenericRecordHeader.from_buffer(data)

        if header.RecordID == records['PMT_RECORD']:
            # cast to PmtEventRecord struct
            event_record = PmtEventRecord.from_buffer(data,sizeof(GenericRecordHeader))
            # attach the data buffer so it doesn't get garbage collected
            event_record.data = data
            return event_record
        elif header.RecordID in records.values():
            raise NotImplementedError("Unable to decode record type '{name}'".format(
                name=records.keys()[records.values().index(header.RecordID)]))
        else:
            raise TypeError("Unknown record type")

    def recv(self, block=True):
        """Returns the next record from the dispatcher as a string buffer"""
        nbytes = ctypes.c_int()
        dtag = ctypes.create_string_buffer(TAGSIZE+1)
        ctypes.memset(byref(dtag),0,TAGSIZE+1)

        if block:
            rc = self.libconthost.wait_head(dtag, byref(nbytes))
        else:
            rc = self.libconthost.check_head(dtag, byref(nbytes))
            if not rc:
                return None

        data = ctypes.create_string_buffer(nbytes.value)

        if rc > 0:
            if nbytes.value < BUFFER_SIZE:
                rc = self.libconthost.get_data(data, nbytes)
            else:
                raise Exception("Insufficient buffer size")

        return data

    def __iter__(self):
        while True:
            yield self.recv()

    def __del__(self):
        self.libconthost.drop_connection()

