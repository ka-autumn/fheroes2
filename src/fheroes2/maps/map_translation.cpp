/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2026                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "map_translation.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "game_language.h"
#include "logging.h"
#include "maps_fileinfo.h"
#include "settings.h"
#include "system.h"
#include "ui_language.h"

namespace
{
    struct MapTranslationRecord
    {
        std::string fileName;
        std::string sourceName;
        std::string sourceDescription;
        std::string translatedName;
        std::string translatedDescription;
    };

    bool splitCatalogLine( const std::string & line, std::array<std::string, 5> & fields )
    {
        size_t fieldBegin = 0;

        for ( size_t fieldIndex = 0; fieldIndex < fields.size(); ++fieldIndex ) {
            const size_t separator = line.find( '\t', fieldBegin );
            if ( fieldIndex + 1 == fields.size() ) {
                if ( separator != std::string::npos ) {
                    return false;
                }

                fields[fieldIndex] = line.substr( fieldBegin );
                return true;
            }

            if ( separator == std::string::npos ) {
                return false;
            }

            fields[fieldIndex] = line.substr( fieldBegin, separator - fieldBegin );
            fieldBegin = separator + 1;
        }

        return false;
    }

    std::vector<MapTranslationRecord> loadJapaneseMapTranslations()
    {
        constexpr const char * catalogFileName = "ja.map-translation.tsv";

#if defined( MACOS_APP_BUNDLE )
        const ListFiles catalogs = Settings::FindFiles( "translations", catalogFileName, false );
#else
        const ListFiles catalogs = Settings::FindFiles( System::concatPath( "files", "lang" ), catalogFileName, false );
#endif

        if ( catalogs.empty() ) {
            ERROR_LOG( "Map translation catalog " << catalogFileName << " was not found." )
            return {};
        }

        std::ifstream input( catalogs.back(), std::ios::binary );
        if ( !input ) {
            ERROR_LOG( "Unable to open map translation catalog '" << catalogs.back() << "'." )
            return {};
        }

        std::vector<MapTranslationRecord> records;
        std::string line;
        size_t lineNumber = 0;
        while ( std::getline( input, line ) ) {
            ++lineNumber;
            if ( !line.empty() && line.back() == '\r' ) {
                line.pop_back();
            }
            if ( line.empty() || line.front() == '#' ) {
                continue;
            }

            std::array<std::string, 5> fields;
            if ( !splitCatalogLine( line, fields ) ) {
                ERROR_LOG( "Invalid map translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }
            if ( std::any_of( fields.cbegin(), fields.cend(), []( const std::string & field ) { return field.empty(); } ) ) {
                ERROR_LOG( "Empty map translation catalog field at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            const bool duplicate = std::any_of( records.cbegin(), records.cend(), [&fields]( const MapTranslationRecord & record ) {
                return record.fileName == fields[0] && record.sourceName == fields[1] && record.sourceDescription == fields[2];
            } );
            if ( duplicate ) {
                ERROR_LOG( "Duplicate map translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            records.push_back( { std::move( fields[0] ), std::move( fields[1] ), std::move( fields[2] ), std::move( fields[3] ), std::move( fields[4] ) } );
        }

        return records;
    }
}

Maps::MapTextForDisplay Maps::resolveMapTextForDisplay( const FileInfo & info )
{
    if ( info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return { info.name, info.description, info.getSupportedLanguage() };
    }

    static const std::vector<MapTranslationRecord> records = loadJapaneseMapTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    const auto record = std::find_if( records.cbegin(), records.cend(), [&fileName, &info]( const MapTranslationRecord & candidate ) {
        return candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description;
    } );
    if ( record != records.cend() ) {
        return { record->translatedName, record->translatedDescription, fheroes2::SupportedLanguage::Japanese };
    }

    static std::set<std::string> loggedMapKeys;
    const std::string mapKey = fileName + '\n' + info.name + '\n' + info.description;
    if ( loggedMapKeys.emplace( mapKey ).second ) {
        VERBOSE_LOG( "No Japanese map translation matched the composite key for '" << fileName << "'. Displaying the original text." )
    }

    return { info.name, info.description, info.getSupportedLanguage() };
}
