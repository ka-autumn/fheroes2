/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#pragma once

#include <optional>
#include <string>

#include "game_language.h"

namespace Maps
{
    struct FileInfo;

    struct MapTextForDisplay
    {
        const std::string & name;
        const std::string & description;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    // The canonical FileInfo strings are deliberately not replaced because persistence and map identity depend on them.
    MapTextForDisplay resolveMapTextForDisplay( const FileInfo & info );
}
