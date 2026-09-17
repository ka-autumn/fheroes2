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

    struct MapEventTranslationRecord
    {
        std::string fileName;
        std::string sourceName;
        std::string sourceDescription;
        std::string sourceEventText;
        std::string translatedEventText;
    };

    struct MapObjectTranslationRecord
    {
        std::string fileName;
        std::string sourceName;
        std::string sourceDescription;
        Maps::MapObjectTextKind textKind;
        std::string sourceObjectText;
        std::string translatedObjectText;
    };

    template<size_t fieldCount>
    bool splitCatalogLine( const std::string & line, std::array<std::string, fieldCount> & fields )
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

    std::optional<Maps::MapObjectTextKind> parseMapObjectTextKind( const std::string & value )
    {
        if ( value == "castle-name" ) {
            return Maps::MapObjectTextKind::CastleName;
        }
        if ( value == "hero-name" ) {
            return Maps::MapObjectTextKind::HeroName;
        }
        if ( value == "sign-message" ) {
            return Maps::MapObjectTextKind::SignMessage;
        }
        if ( value == "bottle-message" ) {
            return Maps::MapObjectTextKind::BottleMessage;
        }

        return std::nullopt;
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

    std::vector<MapEventTranslationRecord> loadJapaneseMapEventTranslations()
    {
        constexpr const char * catalogFileName = "ja.map-event-translation.tsv";

#if defined( MACOS_APP_BUNDLE )
        const ListFiles catalogs = Settings::FindFiles( "translations", catalogFileName, false );
#else
        const ListFiles catalogs = Settings::FindFiles( System::concatPath( "files", "lang" ), catalogFileName, false );
#endif

        if ( catalogs.empty() ) {
            ERROR_LOG( "Map event translation catalog " << catalogFileName << " was not found." )
            return {};
        }

        std::ifstream input( catalogs.back(), std::ios::binary );
        if ( !input ) {
            ERROR_LOG( "Unable to open map event translation catalog '" << catalogs.back() << "'." )
            return {};
        }

        std::vector<MapEventTranslationRecord> records;
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
                ERROR_LOG( "Invalid map event translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }
            if ( std::any_of( fields.cbegin(), fields.cend(), []( const std::string & field ) { return field.empty(); } ) ) {
                ERROR_LOG( "Empty map event translation catalog field at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            const bool duplicate = std::any_of( records.cbegin(), records.cend(), [&fields]( const MapEventTranslationRecord & record ) {
                return record.fileName == fields[0] && record.sourceName == fields[1] && record.sourceDescription == fields[2] && record.sourceEventText == fields[3];
            } );
            if ( duplicate ) {
                ERROR_LOG( "Duplicate map event translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            records.push_back( { std::move( fields[0] ), std::move( fields[1] ), std::move( fields[2] ), std::move( fields[3] ), std::move( fields[4] ) } );
        }

        return records;
    }

    std::vector<MapObjectTranslationRecord> loadJapaneseMapObjectTranslations()
    {
        constexpr const char * catalogFileName = "ja.map-object-translation.tsv";

#if defined( MACOS_APP_BUNDLE )
        const ListFiles catalogs = Settings::FindFiles( "translations", catalogFileName, false );
#else
        const ListFiles catalogs = Settings::FindFiles( System::concatPath( "files", "lang" ), catalogFileName, false );
#endif

        if ( catalogs.empty() ) {
            ERROR_LOG( "Map object translation catalog " << catalogFileName << " was not found." )
            return {};
        }

        std::ifstream input( catalogs.back(), std::ios::binary );
        if ( !input ) {
            ERROR_LOG( "Unable to open map object translation catalog '" << catalogs.back() << "'." )
            return {};
        }

        std::vector<MapObjectTranslationRecord> records;
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

            std::array<std::string, 6> fields;
            if ( !splitCatalogLine( line, fields ) ) {
                ERROR_LOG( "Invalid map object translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }
            if ( std::any_of( fields.cbegin(), fields.cend(), []( const std::string & field ) { return field.empty(); } ) ) {
                ERROR_LOG( "Empty map object translation catalog field at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            const std::optional<Maps::MapObjectTextKind> textKind = parseMapObjectTextKind( fields[3] );
            if ( !textKind ) {
                ERROR_LOG( "Unknown map object text kind at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            const bool duplicate = std::any_of( records.cbegin(), records.cend(), [&fields, textKind]( const MapObjectTranslationRecord & record ) {
                return record.fileName == fields[0] && record.sourceName == fields[1] && record.sourceDescription == fields[2] && record.textKind == *textKind
                       && record.sourceObjectText == fields[4];
            } );
            if ( duplicate ) {
                ERROR_LOG( "Duplicate map object translation catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            records.push_back(
                { std::move( fields[0] ), std::move( fields[1] ), std::move( fields[2] ), *textKind, std::move( fields[4] ), std::move( fields[5] ) } );
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

Maps::MapEventTextForDisplay Maps::resolveMapEventTextForDisplay( const FileInfo & info, const std::string & sourceEventText )
{
    if ( info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return { sourceEventText, info.getSupportedLanguage() };
    }

    static const std::vector<MapEventTranslationRecord> records = loadJapaneseMapEventTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    const auto record = std::find_if( records.cbegin(), records.cend(), [&fileName, &info, &sourceEventText]( const MapEventTranslationRecord & candidate ) {
        return candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description
               && candidate.sourceEventText == sourceEventText;
    } );
    if ( record != records.cend() ) {
        return { record->translatedEventText, fheroes2::SupportedLanguage::Japanese };
    }

    static std::set<std::string> loggedEventKeys;
    const std::string eventKey = fileName + '\n' + info.name + '\n' + info.description + '\n' + sourceEventText;
    if ( loggedEventKeys.emplace( eventKey ).second ) {
        VERBOSE_LOG( "No Japanese map event translation matched the composite key for '" << fileName << "'. Displaying the original text." )
    }

    return { sourceEventText, info.getSupportedLanguage() };
}

Maps::MapObjectTextForDisplay Maps::resolveMapObjectTextForDisplay( const FileInfo & info, const MapObjectTextKind textKind,
                                                                   const std::string & sourceObjectText )
{
    if ( info.filename.empty() || info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return { sourceObjectText, info.getSupportedLanguage() };
    }

    static const std::vector<MapObjectTranslationRecord> records = loadJapaneseMapObjectTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    const auto record
        = std::find_if( records.cbegin(), records.cend(), [&fileName, &info, textKind, &sourceObjectText]( const MapObjectTranslationRecord & candidate ) {
              return candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description
                     && candidate.textKind == textKind && candidate.sourceObjectText == sourceObjectText;
          } );
    if ( record != records.cend() ) {
        return { record->translatedObjectText, fheroes2::SupportedLanguage::Japanese };
    }

    return { sourceObjectText, info.getSupportedLanguage() };
}
