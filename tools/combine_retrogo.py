#!/usr/bin/env python3
"""Junta a imagem do Retro-Go e o firmware próprio do d-BMO numa imagem só da flash.

O Retro-Go (firmware/retro-go/build.sh build-img) gera uma imagem crua da flash: bootloader em
0x0, tabela de partições em 0x8000, as partições dos apps e, depois do fim delas, um rodapé de
256 bytes ("RG_IMG_0") do atualizador. Este script acrescenta à tabela a partição "dbmo" (app,
subtipo factory), grava nela o firmware.bin do PlatformIO e escreve a imagem sem o rodapé, para
gravar inteira em 0x0:

    esptool.py --chip esp32s3 write-flash --flash-size detect 0x0 dbmo-combinado.bin

Com a otadata em branco, o bootloader liga a partição factory: a placa começa no d-BMO. O item
Jogos do menu aponta o boot ao "launcher"; o alvo dbmo-v1 do Retro-Go devolve o boot ao "dbmo"
em todo boot, e um reset volta ao d-BMO.

Uso: tools/combine_retrogo.py --retrogo RETRO-GO.img --dbmo firmware.bin -o SAIDA.bin
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

TABLE_OFFSET = 0x8000
TABLE_MAX = 0xC00  # o que o ESP-IDF lê da tabela, já com o MD5
ENTRY = struct.Struct("<2sBBII16sI")
ENTRY_MAGIC = b"\xaa\x50"
MD5_MAGIC = b"\xeb\xeb" + b"\xff" * 14
APP_ALIGN = 0x10000
TYPE_APP, TYPE_DATA = 0x00, 0x01
SUBTYPE_FACTORY, SUBTYPE_OTADATA = 0x00, 0x00
ESP_IMAGE_MAGIC = 0xE9
FOOTER_MAGIC = b"RG_IMG_0"


class CombineError(Exception):
    pass


def parse_table(image: bytes) -> list[tuple[int, int, int, int, str, int]]:
    """Entradas (tipo, subtipo, offset, tamanho, nome, flags) da tabela em 0x8000."""
    entries = []
    for pos in range(TABLE_OFFSET, TABLE_OFFSET + TABLE_MAX, ENTRY.size):
        chunk = image[pos : pos + ENTRY.size]
        if len(chunk) < ENTRY.size or chunk[:2] in (b"\xff\xff", MD5_MAGIC[:2]):
            break
        magic, ptype, subtype, offset, size, label, flags = ENTRY.unpack(chunk)
        if magic != ENTRY_MAGIC:
            raise CombineError(f"entrada inválida em 0x{pos:x}: não é uma tabela de partições")
        entries.append((ptype, subtype, offset, size, label.rstrip(b"\x00").decode(), flags))
    if not entries:
        raise CombineError("tabela de partições vazia em 0x8000")
    return entries


def build_table(entries) -> bytes:
    """Tabela binária no formato do ESP-IDF: entradas, marcador com MD5 e 0xFF até 0xC00."""
    body = b"".join(
        ENTRY.pack(ENTRY_MAGIC, ptype, subtype, offset, size, label.encode(), flags)
        for ptype, subtype, offset, size, label, flags in entries
    )
    table = body + MD5_MAGIC + hashlib.md5(body).digest()
    if len(table) > TABLE_MAX:
        raise CombineError("partições demais para a tabela")
    return table + b"\xff" * (TABLE_MAX - len(table))


def combine(retrogo: bytes, app: bytes, label: str = "dbmo", size: int = 0x200000,
            flash_size: int = 16 * 1024 * 1024) -> tuple[bytes, int]:
    """Imagem combinada e o offset da partição nova."""
    if not app or app[0] != ESP_IMAGE_MAGIC:
        raise CombineError("o firmware não começa com 0xE9: não é uma imagem de app do ESP32")
    if len(app) > size:
        raise CombineError(f"firmware de {len(app)} bytes não cabe em {size} bytes")
    entries = parse_table(retrogo)
    if any(e[4] == label for e in entries):
        raise CombineError(f'a tabela já tem uma partição "{label}"')
    if any(e[0] == TYPE_APP and e[1] == SUBTYPE_FACTORY for e in entries):
        raise CombineError("a tabela já tem uma partição factory")

    end = max(offset + psize for _, _, offset, psize, _, _ in entries)
    offset = (end + APP_ALIGN - 1) // APP_ALIGN * APP_ALIGN
    if offset + size > flash_size:
        raise CombineError(f"a partição nova passaria do fim da flash (0x{flash_size:x})")

    # Até o fim das partições é a flash; o que vier depois (o rodapé RG_IMG_0) fica de fora.
    flash = bytearray(retrogo[:end])
    flash += b"\xff" * (offset - len(flash))
    flash += app
    entries.append((TYPE_APP, SUBTYPE_FACTORY, offset, size, label, 0))
    flash[TABLE_OFFSET : TABLE_OFFSET + TABLE_MAX] = build_table(entries)
    # otadata em branco: o bootloader escolhe a factory, e o aparelho liga no d-BMO.
    for ptype, subtype, poffset, psize, _, _ in entries:
        if ptype == TYPE_DATA and subtype == SUBTYPE_OTADATA:
            flash[poffset : poffset + psize] = b"\xff" * psize
    return bytes(flash), offset


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--retrogo", required=True, type=Path, help="imagem .img do Retro-Go")
    parser.add_argument("--dbmo", required=True, type=Path, help="firmware.bin do PlatformIO")
    parser.add_argument("-o", "--output", required=True, type=Path)
    parser.add_argument("--size", type=lambda v: int(v, 0), default=0x200000,
                        help="tamanho da partição dbmo (padrão 0x200000)")
    args = parser.parse_args(argv)
    retrogo = args.retrogo.read_bytes()
    app = args.dbmo.read_bytes()
    try:
        image, offset = combine(retrogo, app, size=args.size)
    except CombineError as error:
        print(f"combine_retrogo: {error}", file=sys.stderr)
        return 1
    args.output.write_bytes(image)
    footer = FOOTER_MAGIC in retrogo[-256:]
    print(f"partição dbmo: offset 0x{offset:x}, {args.size} bytes, firmware de {len(app)} bytes")
    print(f"imagem: {args.output} ({len(image)} bytes; rodapé RG_IMG_0 {'removido' if footer else 'ausente'})")
    for ptype, subtype, poffset, psize, label, _ in parse_table(image):
        print(f"  {label:<12} tipo {ptype} subtipo 0x{subtype:02x}  0x{poffset:06x}  {psize:>8}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
