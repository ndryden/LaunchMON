"""A CTypes interface to the LaunchMON back-end library."""

from ctypes import *
import lmon

class LMON_be(object):
    """An interface to the LaunchMON back-end library using CTypes.

    This loads the library, provides for type-checking of arguments, and handles
    some convenience things. See the LaunchMON manpages for additional
    information.

    """

    def __init__(self):
        """Initialize the LaunchMON back-end library."""
        self.lib = cdll.LoadLibrary(lmon.lmon_be_lib)
        self.pack_type = CFUNCTYPE(c_int, c_void_p, c_void_p, c_int,
                                   POINTER(c_int))
        self.unpack_type = CFUNCTYPE(c_int, c_void_p, c_int, c_void_p)
        self.pack_cb = None
        self.unpack_cb = None
        # We use c_void_p because ctypes isn't good at the whole
        # multiple-pointers thing.
        self.lib.LMON_be_init.argtypes = [c_int, POINTER(c_int), c_void_p]
        self.lib.LMON_be_getMyRank.argtypes = [POINTER(c_int)]
        self.lib.LMON_be_amIMaster.argtypes = []
        self.lib.LMON_be_getSize.argtypes = [POINTER(c_int)]
        self.lib.LMON_be_handshake.argtypes = [c_void_p]
        self.lib.LMON_be_ready.argtypes = [c_void_p]
        self.lib.LMON_be_getMyProctabSize.argtypes = [POINTER(c_int)]
        # See above for why we use c_void_p.
        self.lib.LMON_be_getMyProctab.argtypes = [c_void_p, POINTER(c_int),
                                                  c_int]
        self.lib.LMON_be_finalize.argtypes = []
        self.lib.LMON_be_regPackForBeToFe.argtypes = [self.pack_type]
        self.lib.LMON_be_regUnpackForFeToBe.argtypes = [self.unpack_type]
        self.lib.LMON_be_sendUsrData.argtypes = [c_void_p]
        self.lib.LMON_be_recvUsrData.argtypes = [c_void_p]
        self.lib.LMON_be_barrier.argtypes = []
        self.lib.LMON_be_broadcast.argtypes = [c_void_p, c_int]
        self.lib.LMON_be_scatter.argtypes = [c_void_p, c_int, c_void_p]

    def init(self, argc, argv):
        """Invoke LMON_be_init.

        argc is the number of arguments in argv.
        argv is a list of arguments to pass as argv.

        """
        _argc = c_int(argc)
        # This is horribly ugly code to properly reconstruct argv for LaunchMON.
        # We stuff the arguments in string buffers (since they can be modified).
        # We stuff those into an array.
        # We add an entry at the end with a bunch of null bytes, since the last
        # argv entry is supposed to be a null and otherwise LaunchMON will make
        # malloc *very* unhappy.
        # We create a pointer to this array.
        # We pass that pointer by reference (another pointer).
        tmp_argv = [cast(create_string_buffer(x), c_char_p) for x in argv]
        tmp_argv.append(cast(create_string_buffer(32), c_char_p))
        _argv = lmon.create_array(c_char_p, tmp_argv)
        argv_ref = c_void_p(addressof(_argv))
        lmon.call(self.lib.LMON_be_init, lmon.LMON_VERSION, byref(_argc),
                  byref(argv_ref))

    def getMyRank(self):
        """Return the rank of this process."""
        rank = c_int()
        lmon.call(self.lib.LMON_be_getMyRank, byref(rank))
        return rank.value

    def amIMaster(self):
        """Return whether this process is the master."""
        rc = lmon.call(self.lib.LMON_be_amIMaster)
        if rc == lmon.LMON_YES:
            return True
        return False

    def getSize(self):
        """Return the number of LaunchMON processes."""
        size = c_int()
        lmon.call(self.lib.LMON_be_getSize, byref(size))
        return size.value

    def handshake(self, udata):
        """Do the LaunchMON handshake.

        If udata is not None, it should be the length of the buffer to
        unserialize front-end data to.

        """
        if udata is None:
            lmon.call(self.lib.LMON_be_handshake, cast(None, c_void_p))
        else:
            buf = create_string_buffer(udata)
            lmon.call(self.lib.LMON_be_handshake, cast(buf, c_void_p))
            # Check if we actually received data.
            if buf.raw[0] != "\0":
                return lmon.udata_unserialize(buf.value)
            return None

    def ready(self, udata):
        """Inform the front-end that we are ready."""
        if udata is None:
            lmon.call(self.lib.LMON_be_ready, cast(None, c_void_p))
        else:
            lmon.call(self.lib.LMON_be_ready, lmon.udata_serialize(udata))

    def getMyProctabSize(self):
        """Return the size of the process table."""
        size = c_int()
        lmon.call(self.lib.LMON_be_getMyProctabSize, byref(size))
        return size.value

    def getMyProctab(self, maxsize):
        """Return the process table and size for this process."""
        proctab_type = lmon.MPIR_PROCDESC_EXT * maxsize
        proctab = proctab_type()
        size = c_int()
        lmon.call(self.lib.LMON_be_getMyProctab, byref(proctab), byref(size),
                  maxsize)
        return proctab, size.value

    def finalize(self):
        """Finalize this session."""
        lmon.call(self.lib.LMON_be_finalize)

    def regPackForBeToFe(self, callback):
        """Register a pack function."""
        self.pack_cb = self.pack_type(callback)
        lmon.call(self.lib.LMON_be_regPackForBeToFe, self.pack_cb)

    def regUnpackForFeToBe(self, callback):
        """Register an unpack function."""
        self.unpack_cb = self.unpack_type(callback)
        lmon.call(self.lib.LMON_be_regUnpackForFeToBe, self.unpack_cb)

    def sendUsrData(self, udata):
        """Send data to the front-end."""
        lmon.call(self.lib.LMON_be_sendUsrData, lmon.udata_serialize(udata))

    def recvUsrData(self, buf_size):
        """Receive data from the front-end."""
        udata = create_string_buffer(buf_size)
        lmon.call(self.lib.LMON_be_recvUsrData, cast(udata, c_void_p))
        if self.amIMaster():
            return lmon.udata_unserialize(udata.raw)
        return None

    def barrier(self):
        """Make a barrier."""
        lmon.call(self.lib.LMON_be_barrier)

    def broadcast(self, udata, size):
        """Broadcast data.

        The master provides data and the size of it, and this returns None.
        The slaves can provide None for the data, and size is the size of the
        buffer, which is returned after unserialization.

        """
        if self.amIMaster():
            # Master sends the data, returns None.
            lmon.call(self.lib.LMON_be_broadcast, lmon.udata_serialize(udata),
                      size)
            return None
        else:
            # Slave returns the data.
            buf = create_string_buffer(size)
            lmon.call(self.lib.LMON_be_broadcast, cast(buf, c_void_p), size)
            # Depending on the application, there appears to be a spurious null
            # byte at the start of the data. I do not know why. This avoids it.
            if buf.raw[0] == "\0" and buf.raw[1] != "\0":
                buf = string_at(addressof(buf) + 1)
            else:
                buf = buf.value
            return lmon.udata_unserialize(buf)

    def scatter(self, udata_array, elem_size):
        """Scatter data.

        The master provides udata_array, which is an array of data to scatter.
        The slaves may provide None for the data.
        elem_size is the size of each element. Due to serialization of data,
        this should be the maximum size of the data, and the elements are padded
        to this length.

        All callers return their respective data (including the master).

        """
        buf = create_string_buffer(elem_size)
        if self.amIMaster():
            # Master pads data if needed and sends.
            total_size = len(udata_array) * elem_size
            send_buf = create_string_buffer(total_size)
            buf_addr = addressof(send_buf)
            idx = 0
            for elem in udata_array:
                tmp_buf = create_string_buffer(
                    string_at(lmon.udata_serialize(elem)), elem_size)
                memmove(buf_addr + (idx * elem_size), tmp_buf, elem_size)
                idx += 1
            lmon.call(self.lib.LMON_be_scatter, cast(send_buf, c_void_p),
                      elem_size, cast(buf, c_void_p))
        else:
            lmon.call(self.lib.LMON_be_scatter, cast(None, c_void_p), elem_size,
                      cast(buf, c_void_p))
        # This is here for the same reason as in broadcast.
        # Note that it appears that the null byte is only present for children.
        if buf.raw[0] == "\0" and buf.raw[1] != "\0":
            buf = string_at(addressof(buf) + 1)
        else:
            buf = buf.value
        return lmon.udata_unserialize(buf)
