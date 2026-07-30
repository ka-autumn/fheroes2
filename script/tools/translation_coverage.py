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

# The .po files are parsed here instead of being passed to "msgfmt --statistics",
# because gettext cannot be assumed to be present on Windows, which is a supported
# development platform for this project
MSGID = re.compile(r'^msgid\s+"(.*)"')
MSGSTR = re.compile(r'^msgstr(?:\[[0-9]+\])?\s+"(.*)"')
CONTINUATION = re.compile(r'^"(.*)"')


def count_entries(path):
    total = 0
    translated = 0

    # .po files are always UTF-8, do not let the locale of the system decide
    with open(path, "r", encoding="utf-8") as handle:
        # The header of a .po file is its very first entry. It cannot be recognized by
        # an empty msgid, because a regular entry whose text is spread over several
        # lines also starts with an empty msgid
        is_header = True
        in_entry = False
        # Continuation lines belong to the msgstr only right after a msgstr line
        in_msgstr = False
        entry_is_translated = False

        for line in handle:
            line = line.strip()

            if MSGID.match(line):
                if in_entry:
                    if is_header:
                        is_header = False
                    else:
                        total += 1

                        if entry_is_translated:
                            translated += 1

                in_entry = True
                in_msgstr = False
                entry_is_translated = False
                continue

            if not in_entry:
                continue

            msgstr_match = MSGSTR.match(line)

            if msgstr_match:
                if in_msgstr:
                    # A plural entry has one msgstr per plural form, and it is
                    # translated only when none of these forms is empty
                    entry_is_translated = (
                        entry_is_translated and msgstr_match.group(1) != ""
                    )
                else:
                    entry_is_translated = msgstr_match.group(1) != ""

                in_msgstr = True
                continue

            continuation_match = CONTINUATION.match(line)

            if in_msgstr and continuation_match:
                if continuation_match.group(1) != "":
                    entry_is_translated = True

                continue

            in_msgstr = False

        if in_entry and not is_header:
            total += 1

            if entry_is_translated:
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
