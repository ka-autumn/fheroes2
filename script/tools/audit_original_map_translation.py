#!/usr/bin/env python3

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2026                                                    #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

import argparse
import struct
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


MAP_INFO_SIZE = 428
TILE_SIZE = 20
CASTLE_COUNT = 72
CASTLE_POSITION_SIZE = 3
CAPTURE_OBJECT_COUNT = 144
CAPTURE_OBJECT_POSITION_SIZE = 3
ADDON_SIZE = 15

OBJECT_SIGN = 130
OBJECT_EVENT = 147
OBJECT_CASTLE = 163
OBJECT_RANDOM_TOWN = 176
OBJECT_RANDOM_CASTLE = 177
OBJECT_HERO = 183
OBJECT_SPHINX = 207
OBJECT_BOTTLE = 221
OBJECT_JAIL = 251

EXTENDED_METADATA_OBJECTS = {
    OBJECT_SIGN,
    OBJECT_EVENT,
    OBJECT_CASTLE,
    OBJECT_RANDOM_TOWN,
    OBJECT_RANDOM_CASTLE,
    OBJECT_HERO,
    OBJECT_SPHINX,
    OBJECT_BOTTLE,
    OBJECT_JAIL,
}

OBJECT_KIND_BY_CATEGORY = {
    "castle-name": "castle-name",
    "hero-name": "hero-name",
    "sign-message": "sign-message",
    "bottle-message": "bottle-message",
}


@dataclass(frozen=True)
class MapIdentity:
    file_name: str
    source_name: str
    source_description: str


@dataclass(frozen=True)
class TextEntry:
    category: str
    source_text: str
    location: str


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def decode_c_string(data: bytes, offset: int, limit: int | None = None) -> str:
    end_limit = len(data) if limit is None else min(len(data), offset + limit)
    end = data.find(b"\0", offset, end_limit)
    if end < 0:
        end = end_limit
    return data[offset:end].decode("cp1252")


def map_identity(map_path: Path, data: bytes) -> MapIdentity:
    return MapIdentity(
        file_name=map_path.name,
        source_name=decode_c_string(data, 58, 16),
        source_description=decode_c_string(data, 118, 200),
    )


def map_object_links(data: bytes, width: int, height: int) -> dict[int, tuple[int, int]]:
    links: dict[int, tuple[int, int]] = {}
    for tile_index in range(width * height):
        offset = MAP_INFO_SIZE + tile_index * TILE_SIZE
        object_type = data[offset + 9]
        if object_type not in EXTENDED_METADATA_OBJECTS:
            continue

        metadata_index = ((data[offset + 5] << 8) + data[offset + 4]) >> 3
        if metadata_index > 0:
            links[metadata_index] = (tile_index, object_type)

    return links


def information_blocks(data: bytes, width: int, height: int) -> list[bytes]:
    position = MAP_INFO_SIZE + width * height * TILE_SIZE
    addon_count = read_u32(data, position)
    position += 4 + addon_count * ADDON_SIZE
    position += CASTLE_COUNT * CASTLE_POSITION_SIZE
    position += CAPTURE_OBJECT_COUNT * CAPTURE_OBJECT_POSITION_SIZE
    position += 1

    block_count = 0
    while True:
        low = data[position]
        high = data[position + 1]
        position += 2
        if low == 0 and high == 0:
            break
        block_count = high * 256 + low - 1

    blocks: list[bytes] = []
    for _ in range(block_count):
        block_size = read_u16(data, position)
        position += 2
        blocks.append(data[position : position + block_size])
        position += block_size

    if position + 4 != len(data):
        raise ValueError("information blocks do not end at the MP2 UID counter")

    return blocks


def coordinates(tile_index: int, width: int) -> str:
    return f"({tile_index % width},{tile_index // width})"


def extract_text_entries(map_path: Path) -> tuple[MapIdentity, list[TextEntry]]:
    data = map_path.read_bytes()
    if len(data) < MAP_INFO_SIZE or data[:4] != b"\\\0\0\0":
        raise ValueError(f"{map_path} is not an MP2/MX2 map")

    width = read_u32(data, 420)
    height = read_u32(data, 424)
    if width == 0 or width != height:
        raise ValueError(f"unsupported map dimensions: {width}x{height}")

    identity = map_identity(map_path, data)
    entries = [
        TextEntry("map-name", identity.source_name, "map header"),
        TextEntry("map-description", identity.source_description, "map header"),
    ]

    links = map_object_links(data, width, height)
    blocks = information_blocks(data, width, height)

    for block_number, block in enumerate(blocks, start=1):
        link = links.get(block_number)
        if link is None:
            if len(block) >= 50 and block[0] == 0 and block[42] == 1:
                message = decode_c_string(block, 49)
                if message:
                    entries.append(TextEntry("daily-event", message, f"day {read_u16(block, 33)}"))
            elif len(block) >= 9 and block[0] == 0:
                rumor = decode_c_string(block, 8)
                if rumor:
                    entries.append(TextEntry("rumor", rumor, f"information block {block_number}"))
            continue

        tile_index, object_type = link
        location = coordinates(tile_index, width)

        if object_type == OBJECT_EVENT and len(block) >= 50 and block[0] == 1:
            message = decode_c_string(block, 49)
            if message:
                entries.append(TextEntry("ground-event", message, location))
        elif object_type == OBJECT_SIGN and len(block) >= 10 and block[0] == 1:
            message = decode_c_string(block, 9)
            if message:
                entries.append(TextEntry("sign-message", message, location))
        elif object_type == OBJECT_BOTTLE and len(block) >= 10 and block[0] == 1:
            message = decode_c_string(block, 9)
            if message:
                entries.append(TextEntry("bottle-message", message, location))
        elif object_type in {OBJECT_CASTLE, OBJECT_RANDOM_TOWN, OBJECT_RANDOM_CASTLE} and len(block) == 70 and block[24] != 0:
            custom_name = decode_c_string(block, 25, 13)
            if custom_name:
                entries.append(TextEntry("castle-name", custom_name, location))
        elif object_type in {OBJECT_HERO, OBJECT_JAIL} and len(block) == 76 and block[45] != 0:
            custom_name = decode_c_string(block, 46, 13)
            if custom_name:
                entries.append(TextEntry("hero-name", custom_name, location))
        elif object_type == OBJECT_SPHINX and len(block) >= 137 and block[0] == 0:
            answer_count = min(block[31], 8)
            for answer_index in range(answer_count):
                answer = decode_c_string(block, 32 + answer_index * 13, 13)
                if answer:
                    entries.append(TextEntry("sphinx-answer", answer, location))
            riddle = decode_c_string(block, 136)
            if riddle:
                entries.append(TextEntry("sphinx-riddle", riddle, location))

    return identity, entries


def read_tsv(path: Path, expected_columns: int) -> list[list[str]]:
    rows: list[list[str]] = []
    with path.open(encoding="utf-8", newline="") as catalog:
        for line_number, raw_line in enumerate(catalog, start=1):
            line = raw_line.removesuffix("\n").removesuffix("\r")
            if not line or line.startswith("#"):
                continue
            row = line.split("\t")
            if len(row) != expected_columns or any(not field for field in row):
                raise ValueError(f"{path}:{line_number}: expected {expected_columns} non-empty columns")
            rows.append(row)
    return rows


def catalog_keys(catalog_directory: Path) -> tuple[set[tuple[str, ...]], set[tuple[str, ...]], set[tuple[str, ...]]]:
    map_rows = read_tsv(catalog_directory / "ja.map-translation.tsv", 5)
    event_rows = read_tsv(catalog_directory / "ja.map-event-translation.tsv", 5)
    object_rows = read_tsv(catalog_directory / "ja.map-object-translation.tsv", 6)

    map_keys = {(row[0], row[1], row[2]) for row in map_rows}
    event_keys = {(row[0], row[1], row[2], row[3]) for row in event_rows}
    object_keys = {(row[0], row[1], row[2], row[3], row[4]) for row in object_rows}

    if len(map_keys) != len(map_rows):
        raise ValueError("ja.map-translation.tsv contains duplicate keys")
    if len(event_keys) != len(event_rows):
        raise ValueError("ja.map-event-translation.tsv contains duplicate keys")
    if len(object_keys) != len(object_rows):
        raise ValueError("ja.map-object-translation.tsv contains duplicate keys")

    return map_keys, event_keys, object_keys


def is_translated(
    identity: MapIdentity,
    entry: TextEntry,
    map_keys: set[tuple[str, ...]],
    event_keys: set[tuple[str, ...]],
    object_keys: set[tuple[str, ...]],
) -> bool | None:
    map_key = (identity.file_name, identity.source_name, identity.source_description)
    if entry.category in {"map-name", "map-description"}:
        return map_key in map_keys
    if entry.category in {"daily-event", "ground-event"}:
        return (*map_key, entry.source_text) in event_keys
    object_kind = OBJECT_KIND_BY_CATEGORY.get(entry.category)
    if object_kind is not None:
        return (*map_key, object_kind, entry.source_text) in object_keys
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit translatable text embedded in one original MP2/MX2 map.")
    parser.add_argument("map", type=Path, help="MP2/MX2 map file")
    parser.add_argument("--catalog-directory", type=Path, default=Path("files/lang"), help="directory containing Japanese sidecar TSV files")
    arguments = parser.parse_args()

    identity, entries = extract_text_entries(arguments.map)
    map_keys, event_keys, object_keys = catalog_keys(arguments.catalog_directory)

    coverage: Counter[str] = Counter()
    missing: list[TextEntry] = []
    unsupported: list[TextEntry] = []
    for entry in entries:
        translated = is_translated(identity, entry, map_keys, event_keys, object_keys)
        coverage[f"{entry.category}:total"] += 1
        if translated is True:
            coverage[f"{entry.category}:translated"] += 1
        elif translated is False:
            missing.append(entry)
        else:
            unsupported.append(entry)

    print(f"Map: {identity.file_name} / {identity.source_name}")
    print("category\ttotal\ttranslated\tuntranslated\tunsupported")
    for category in sorted({entry.category for entry in entries}):
        total = coverage[f"{category}:total"]
        translated = coverage[f"{category}:translated"]
        unsupported_count = sum(entry.category == category for entry in unsupported)
        untranslated = total - translated - unsupported_count
        print(f"{category}\t{total}\t{translated}\t{untranslated}\t{unsupported_count}")

    for heading, uncovered_entries in (("Untranslated", missing), ("Unsupported", unsupported)):
        if uncovered_entries:
            print(f"\n{heading}:")
            for entry in uncovered_entries:
                print(f"- {entry.category} {entry.location}: {entry.source_text}")

    return 1 if missing or unsupported else 0


if __name__ == "__main__":
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(2)
