#!/usr/bin/env python3
"""Teste de tools/combine_retrogo.py com uma imagem sintética no formato do mkfw.py do Retro-Go."""

from __future__ import annotations

import hashlib
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import combine_retrogo as cr  # noqa: E402

# Mesma disposição da imagem de bancada (execução 45724562378 da CI), em escala menor.
PARTS = [
    (1, 0x02, 0x9000, 0x4000, "nvs", 0),
    (1, 0x00, 0xD000, 0x2000, "otadata", 0),
    (1, 0x01, 0xF000, 0x1000, "phy_init", 0),
    (0, 0x10, 0x10000, 0x100000, "launcher", 0),
    (0, 0x11, 0x110000, 0x100000, "retro-core", 0),
    (1, 0x81, 0x210000, 0x20000, "vfs", 0),
]
END = 0x230000


def retrogo_image(footer: bool = True) -> bytes:
    flash = bytearray(b"\xff" * END)
    flash[0:4] = b"\xe9BOO"  # bootloader
    flash[cr.TABLE_OFFSET : cr.TABLE_OFFSET + cr.TABLE_MAX] = cr.build_table(PARTS)
    flash[0x10000:0x10004] = b"\xe9LAU"
    flash[0x110000:0x110004] = b"\xe9COR"
    if footer:
        flash += cr.FOOTER_MAGIC + b"\x00" * 248
    return bytes(flash)


APP = b"\xe9" + bytes(range(256)) * 8


class CombineTest(unittest.TestCase):
    def test_adds_a_factory_partition_after_the_last_one(self):
        image, offset = cr.combine(retrogo_image(), APP)
        self.assertEqual(offset, 0x230000)  # já alinhado a 0x10000
        entries = cr.parse_table(image)
        self.assertEqual(entries[:-1], PARTS)
        self.assertEqual(entries[-1], (0, 0x00, 0x230000, 0x200000, "dbmo", 0))
        self.assertEqual(image[offset : offset + len(APP)], APP)
        self.assertEqual(len(image), offset + len(APP))

    def test_table_md5_matches_the_entries(self):
        image, _ = cr.combine(retrogo_image(), APP)
        table = image[cr.TABLE_OFFSET : cr.TABLE_OFFSET + cr.TABLE_MAX]
        count = len(PARTS) + 1
        body = table[: count * 32]
        marker = table[count * 32 : count * 32 + 32]
        self.assertEqual(marker[:16], cr.MD5_MAGIC)
        self.assertEqual(marker[16:], hashlib.md5(body).digest())
        self.assertEqual(set(table[count * 32 + 32 :]), {0xFF})

    def test_keeps_the_retro_go_apps_and_drops_the_footer(self):
        source = retrogo_image()
        image, _ = cr.combine(source, APP)
        self.assertEqual(image[0:4], b"\xe9BOO")
        self.assertEqual(image[0x10000:0x10004], b"\xe9LAU")
        self.assertEqual(image[0x110000:0x110004], b"\xe9COR")
        self.assertNotIn(cr.FOOTER_MAGIC, image)
        self.assertEqual(image[: cr.TABLE_OFFSET], source[: cr.TABLE_OFFSET])

    def test_otadata_is_blank_so_the_factory_app_boots(self):
        source = bytearray(retrogo_image())
        source[0xD000:0xD020] = b"\x01" * 32  # otadata gravada por um boot anterior
        image, _ = cr.combine(bytes(source), APP)
        self.assertEqual(set(image[0xD000:0xF000]), {0xFF})

    def test_aligns_to_64_kib(self):
        parts = PARTS[:-1] + [(1, 0x81, 0x210000, 0x1800, "vfs", 0)]
        flash = bytearray(retrogo_image(footer=False)[:0x211800])
        flash[cr.TABLE_OFFSET : cr.TABLE_OFFSET + cr.TABLE_MAX] = cr.build_table(parts)
        _, offset = cr.combine(bytes(flash), APP)
        self.assertEqual(offset, 0x220000)

    def test_refuses_what_would_not_boot(self):
        with self.assertRaises(cr.CombineError):
            cr.combine(retrogo_image(), b"\x00" + APP)  # não é imagem de app
        with self.assertRaises(cr.CombineError):
            cr.combine(retrogo_image(), APP, size=len(APP) - 1)  # não cabe
        with self.assertRaises(cr.CombineError):
            cr.combine(retrogo_image(), APP, size=0x200000, flash_size=0x400000)  # passa da flash
        twice, _ = cr.combine(retrogo_image(), APP)
        with self.assertRaises(cr.CombineError):
            cr.combine(twice, APP)  # já tem "dbmo" e uma factory
        with self.assertRaises(cr.CombineError):
            cr.combine(b"\xff" * 0x10000, APP)  # sem tabela


if __name__ == "__main__":
    unittest.main()
