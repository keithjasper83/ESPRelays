import importlib.util
import struct
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("size_guard", Path(__file__).resolve().parents[2] / "scripts/check_firmware_size.py")
guard = importlib.util.module_from_spec(spec)
spec.loader.exec_module(guard)


def partition(kind, subtype, size):
    return struct.pack("<HBBII16sI", 0x50AA, kind, subtype, 0x10000, size, b"test", 0)


class FirmwareSizeTest(unittest.TestCase):
    def test_actual_binary_must_fit_smallest_application_slot(self):
        table = partition(1, 2, 20) + partition(0, 16, 100) + partition(0, 17, 90)
        self.assertEqual(0, guard.validate_image(b"x" * 90, table))
        with self.assertRaisesRegex(ValueError, "91.*90"):
            guard.validate_image(b"x" * 91, table)
        self.assertEqual(10, guard.validate_image(b"x" * 80, table))

    def test_fail_closed_without_valid_application_partition(self):
        for table in (b"", b"truncated", partition(1, 2, 100), partition(0, 16, 0)):
            with self.subTest(table=table), self.assertRaises(ValueError):
                guard.validate_image(b"x", table)

    def test_md5_footer_and_padding_are_not_partitions(self):
        table = partition(0, 16, 100) + b"\xeb\xeb" + b"\xff" * 30 + b"\xff" * 64
        self.assertEqual(95, guard.validate_image(b"12345", table))


if __name__ == "__main__":
    unittest.main()
