import fcntl
import ctypes
import errno
import os


_IOC_NRBITS = 8
_IOC_TYPEBITS = 8
_IOC_SIZEBITS = 14
_IOC_DIRBITS = 2

_IOC_NRSHIFT = 0
_IOC_TYPESHIFT = _IOC_NRSHIFT + _IOC_NRBITS
_IOC_SIZESHIFT = _IOC_TYPESHIFT + _IOC_TYPEBITS
_IOC_DIRSHIFT = _IOC_SIZESHIFT + _IOC_SIZEBITS

_IOC_NONE = 0
_IOC_WRITE = 1
_IOC_READ = 2


def _IOC(dir, ty, nr, size):
    return (dir << _IOC_DIRSHIFT)  \
        | (ty << _IOC_TYPESHIFT)   \
        | (nr << _IOC_NRSHIFT)     \
        | (size << _IOC_SIZESHIFT)


def _IO(ty, nr):
    return _IOC(_IOC_NONE, ty, nr, 0)


def _IOR(ty, nr, size):
    return _IOC(_IOC_READ, ty, nr, size)


def _IOW(ty, nr, size):
    return _IOC(_IOC_WRITE, ty, nr, size)


def _IOWR(ty, nr, size):
    return _IOC(_IOC_WRITE | _IOC_READ, ty, nr, size)


class _RawRequestPayload(ctypes.Structure):
    _fields_ = [
        ('data', ctypes.c_uint64),
        ('length', ctypes.c_uint16),
        ('__pad', ctypes.c_uint8 * 6),
    ]


class _RawRequestResponse(ctypes.Structure):
    _fields_ = [
        ('data', ctypes.c_uint64),
        ('length', ctypes.c_uint16),
        ('__pad', ctypes.c_uint8 * 6),
    ]


class _RawRequest(ctypes.Structure):
    _fields_ = [
        ('target_category', ctypes.c_uint8),
        ('target_id', ctypes.c_uint8),
        ('command_id', ctypes.c_uint8),
        ('instance_id', ctypes.c_uint8),
        ('flags', ctypes.c_uint16),
        ('status', ctypes.c_int16),
        ('payload', _RawRequestPayload),
        ('response', _RawRequestResponse),
    ]


class Request:
    target_category: int
    target_id: int
    command_id: int
    instance_id: int
    flags: int
    payload: bytes
    response_cap: int

    def __init__(self, target_category, target_id, command_id, instance_id,
                 flags=0, payload=bytes(), response_cap=1024):
        self.target_category = target_category
        self.target_id = target_id
        self.command_id = command_id
        self.instance_id = instance_id
        self.flags = flags
        self.payload = payload
        self.response_cap = response_cap


REQUEST_HAS_RESPONSE = 1
REQUEST_UNSEQUENCED = 2


_PATH_SSAM_DBGDEV = '/dev/surface/aggregator'

_IOCTL_REQUEST = _IOWR(0xA5, 1, ctypes.sizeof(_RawRequest))


def _request(fd, rqst: Request):
    # set up basic request fields
    raw = _RawRequest()
    raw.target_category = rqst.target_category
    raw.target_id = rqst.target_id
    raw.command_id = rqst.command_id
    raw.instance_id = rqst.instance_id
    raw.flags = rqst.flags
    raw.status = -errno.ENXIO

    # set up payload
    if rqst.payload:
        pld_type = ctypes.c_uint8 * len(rqst.payload)
        pld_buf = pld_type(*rqst.payload)
        pld_ptr = ctypes.pointer(pld_buf)
        pld_ptr = ctypes.cast(pld_ptr, ctypes.c_void_p)

        raw.payload.data = pld_ptr.value
        raw.payload.length = len(rqst.payload)
    else:
        raw.payload.data = 0
        raw.payload.length = 0

    # set up response
    if rqst.response_cap > 0:
        rsp_cap = rqst.response_cap
        if rsp_cap > 0xffff:
            rsp_cap = 0xffff

        rsp_type = ctypes.c_uint8 * rsp_cap
        rsp_buf = rsp_type()
        rsp_ptr = ctypes.pointer(rsp_buf)
        rsp_ptr = ctypes.cast(rsp_ptr, ctypes.c_void_p)

        raw.response.data = rsp_ptr.value
        raw.response.length = rsp_cap
    else:
        raw.response.data = 0
        raw.response.length = 0

    # perform actual IOCTL
    buf = bytearray(raw)
    fcntl.ioctl(fd, _IOCTL_REQUEST, buf, True)
    raw = _RawRequest.from_buffer(buf)

    if raw.status:
        raise OSError(-raw.status, errno.errorcode.get(-raw.status))

    # convert response to bytes and return
    if raw.response.length > 0:
        return bytes(rsp_buf[:raw.response.length])
    else:
        return None


class Controller:
    def __init__(self):
        self.fd = None

    def open(self):
        self.fd = os.open(_PATH_SSAM_DBGDEV, os.O_RDWR)
        return self

    def close(self):
        os.close(self.fd)

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def request(self, request: Request):
        if self.fd is None:
            raise RuntimeError("controller is not open")

        return _request(self.fd, request)
