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

"""Report translation coverage for every .po file in a directory."""

import argparse
import os
import re
import sys

MSGID = re.compile(r'^msgid\s+"(.*)"')
MSGSTR = re.compile(r'^msgstr(?:\[[0-9]+\])?\s+"(.*)"')
CONTINUATION = re.compile(r'^"(.*)"')


def count_entries(path):
    total = 0
    translated = 0

    with open(path, encoding="utf-8") as handle:
        content = handle.read()

    # Entries are separated by empty lines
    for block in content.split("\n\n"):
        msgid = None
        msgstrs = []
        current = None

        for line in block.splitlines():
            line = line.strip()

            match = MSGID.match(line)
            if match:
                msgid = match.group(1)
                current = MSGID
                continue

            match = MSGSTR.match(line)
            if match:
                msgstrs.append(match.group(1))
                current = MSGSTR
                continue

            # A string containing line breaks is continued on the following lines
            match = CONTINUATION.match(line)
            if match:
                if current is MSGID:
                    msgid += match.group(1)
                elif current is MSGSTR:
                    msgstrs[-1] += match.group(1)

                continue

            # Comments, msgid_plural and obsolete entries do not continue a string
            current = None

        # The header entry of a .po file has an empty msgid, skip it. Blocks
        # without a msgid at all (comments, obsolete entries) are skipped too
        if not msgid:
            continue

        total += 1

        # A plural entry is translated only when every one of its forms is filled in
        if msgstrs and all(msgstrs):
            translated += 1

    return total, translated


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", help="directory containing .po files")
    parser.add_argument("--min", type=int, default=50, help="fail below this percentage")
    args = parser.parse_args()

    failed = False

    for name in sorted(os.listdir(args.directory)):
        if not name.endswith(".po"):
            continue

        total, translated = count_entries(os.path.join(args.directory, name))
        percent = translated * 100 / total

        print("{:<20} {:>5} / {:<5} {:>5.1f}%".format(name, translated, total, percent))

        if percent < args.min:
            failed = True

    if failed:
        sys.exit(1)


if __name__ == "__main__":
    main()
