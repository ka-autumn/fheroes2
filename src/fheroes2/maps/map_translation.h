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

#include <cstdint>
#include <list>
#include <optional>
#include <string>

#include "game_language.h"

namespace Maps
{
    struct FileInfo;

    enum class MapObjectTextKind : uint8_t
    {
        CastleName,
        HeroName,
        SignMessage,
        BottleMessage
    };

    struct MapTextForDisplay
    {
        const std::string & name;
        const std::string & description;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    struct MapEventTextForDisplay
    {
        const std::string & text;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    struct MapObjectTextForDisplay
    {
        const std::string & text;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    struct MapRumorTextForDisplay
    {
        const std::string & text;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    struct MapSphinxRiddleForDisplay
    {
        const std::string & text;
        std::optional<fheroes2::SupportedLanguage> language;
    };

    // The canonical FileInfo strings are deliberately not replaced because persistence and map identity depend on them.
    MapTextForDisplay resolveMapTextForDisplay( const FileInfo & info );

    // The canonical event text is deliberately not replaced because it is persisted in save files.
    MapEventTextForDisplay resolveMapEventTextForDisplay( const FileInfo & info, const std::string & sourceEventText );

    // The canonical object text is deliberately not replaced because custom names and messages are persisted in save files.
    MapObjectTextForDisplay resolveMapObjectTextForDisplay( const FileInfo & info, MapObjectTextKind textKind, const std::string & sourceObjectText );

    // The canonical custom rumor is deliberately not replaced because it is persisted in save files.
    MapRumorTextForDisplay resolveMapRumorTextForDisplay( const FileInfo & info, const std::string & sourceRumorText );

    // The canonical riddle is deliberately not replaced because it is persisted in save files.
    MapSphinxRiddleForDisplay resolveMapSphinxRiddleForDisplay( const FileInfo & info, const std::string & sourceRiddleText );

    // Returns true if the input matches an additional answer registered for one of the canonical answers. The canonical answers are not replaced.
    // The canonical answers are compared ignoring ASCII case because the game lowercases them when a map is loaded, and the additional answers are
    // compared for equality after trimming whitespace and unifying full-width and half-width ASCII characters, and only while
    // the Japanese UI language is active. The caller is still responsible for checking the canonical answers.
    bool isAdditionalSphinxAnswerAccepted( const FileInfo & info, const std::list<std::string> & sourceAnswers, const std::string & input );
}
