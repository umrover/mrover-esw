import struct
from dataclasses import astuple, dataclass
from typing import ClassVar, TypeVar

import serial

from esw.stlink import get_stlinkv3_port

MAX_MESSAGE_SIZE = 254


def _cobs_encode(data: bytes) -> bytes:
    out = bytearray()
    for block in data.split(b"\x00"):
        out += bytes([len(block) + 1]) + block
    return bytes(out)


def _cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    i = 0
    while i < len(data):
        code = data[i]
        if code == 0 or i + code > len(data):
            raise ValueError("malformed frame")
        out += data[i + 1 : i + code]
        i += code
        if code != 0xFF and i < len(data):
            out.append(0)
    return bytes(out)


@dataclass
class Message:
    FORMAT: ClassVar[str]

    def pack(self) -> bytes:
        return struct.pack(self.FORMAT, *astuple(self))

    @classmethod
    def unpack(cls, data: bytes):
        return cls(*struct.unpack(cls.FORMAT, data))


M = TypeVar("M", bound=Message)


class VCP:
    def __init__(self, port: str | None = None, baud: int = 115200, timeout: float = 1.0):
        port = port or get_stlinkv3_port()
        if port is None:
            raise RuntimeError("no ST-LINK VCP found; pass the port explicitly")
        self.ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)

    def __enter__(self):
        self.ser.reset_input_buffer()
        return self

    def __exit__(self, *exc):
        self.ser.close()

    def send(self, msg: Message) -> None:
        data = msg.pack()
        if len(data) > MAX_MESSAGE_SIZE:
            raise ValueError(f"message too large ({len(data)} > {MAX_MESSAGE_SIZE} bytes)")
        self.ser.write(_cobs_encode(data) + b"\x00")

    def receive(self, msg_type: type[M]) -> M | None:
        size = struct.calcsize(msg_type.FORMAT)
        while (frame := self.ser.read_until(b"\x00")).endswith(b"\x00"):
            try:
                data = _cobs_decode(frame[:-1])
            except ValueError:
                continue
            if len(data) == size:
                return msg_type.unpack(data)
        return None
