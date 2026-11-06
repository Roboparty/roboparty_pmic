import importlib.util
import struct
import unittest
import zlib
from pathlib import Path


MODULE = Path(__file__).with_name("stm32_serial_ota.py")
SPEC = importlib.util.spec_from_file_location("stm32_serial_ota", MODULE)
OTA = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(OTA)


def image():
    return struct.pack("<II", 0x20005000, OTA.APP_START + 0x101) + bytes(range(256)) * 5


class FakePort:
    def __init__(self, recover=False):
        self.response = bytearray()
        self.recover = recover
        self.entered = recover
        self.data = bytearray()
        self.expected = 0
        self.expected_crc = 0
        self.finished = False

    def reset_input_buffer(self):
        self.response.clear()

    def write(self, command):
        command = command.decode("ascii")
        if command == "SN_GET:1;":
            self.response.extend(b"CHRG_SN:0669FF555553787287104058;")
        elif command == "OTA_ENTER:1;":
            self.entered = True
            self.response.extend(b"OTA_BOOT;OTA_BOOT:1;")
        elif command == "OTA_PING:1;" and self.entered:
            self.response.extend(b"OTA_BOOT:1;")
        elif command.startswith("OTA_BEGIN:") and self.entered:
            self.expected, self.expected_crc = map(int, command[10:-1].split(","))
            self.data.clear()
            self.response.extend(b"OTA_READY;")
        elif command.startswith("OTA_DATA:") and self.entered:
            offset, encoded = command[9:-1].split(",")
            assert int(offset) == len(self.data)
            self.data.extend(bytes.fromhex(encoded))
            self.response.extend(f"OTA_ACK:{len(self.data)};".encode())
        elif command == "OTA_END:1;":
            assert len(self.data) == self.expected
            assert zlib.crc32(self.data) & 0xFFFFFFFF == self.expected_crc
            self.finished = True
            self.response.extend(b"OTA_DONE;")
        else:
            raise AssertionError(command)

    def flush(self):
        pass

    def read(self, count):
        result = bytes(self.response[:count])
        del self.response[:count]
        return result


class OtaToolTests(unittest.TestCase):
    def test_full_update(self):
        port = FakePort()
        payload = image()
        OTA.update(port, payload, 0.1, uid="0669FF555553787287104058")
        self.assertTrue(port.finished)
        self.assertEqual(port.data, payload)

    def test_recovery_update(self):
        port = FakePort(recover=True)
        OTA.update(port, image(), 0.1, recover=True)
        self.assertTrue(port.finished)

    def test_wrong_uid_does_not_enter(self):
        port = FakePort()
        with self.assertRaises(RuntimeError):
            OTA.update(port, image(), 0.1, uid="000000000000000000000000")
        self.assertFalse(port.entered)

    def test_rejects_wrong_link_address(self):
        with self.assertRaises(ValueError):
            OTA.validate_image(struct.pack("<II", 0x20005000, 0x08000101))


if __name__ == "__main__":
    unittest.main()
