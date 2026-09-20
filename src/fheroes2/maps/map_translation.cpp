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
#include <cstdint>
#include <fstream>
#include <list>
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

    struct MapSphinxTranslationRecord
    {
        std::string fileName;
        std::string sourceName;
        std::string sourceDescription;
        bool isAnswer;
        std::string sourceText;
        std::string translatedText;
    };

    // Reads a Japanese sidecar catalog and passes every well-formed row to `addRecord`, which returns false for a row that must be rejected.
    template<size_t fieldCount, typename AddRecord>
    void readCatalog( const char * catalogFileName, const char * label, AddRecord && addRecord )
    {
#if defined( MACOS_APP_BUNDLE )
        const ListFiles catalogs = Settings::FindFiles( "translations", catalogFileName, false );
#else
        const ListFiles catalogs = Settings::FindFiles( System::concatPath( "files", "lang" ), catalogFileName, false );
#endif

        if ( catalogs.empty() ) {
            ERROR_LOG( label << " catalog " << catalogFileName << " was not found." )
            return;
        }

        std::ifstream input( catalogs.back(), std::ios::binary );
        if ( !input ) {
            ERROR_LOG( "Unable to open " << label << " catalog '" << catalogs.back() << "'." )
            return;
        }

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

            std::array<std::string, fieldCount> fields;
            if ( !splitCatalogLine( line, fields ) ) {
                ERROR_LOG( "Invalid " << label << " catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }
            if ( std::any_of( fields.cbegin(), fields.cend(), []( const std::string & field ) { return field.empty(); } ) ) {
                ERROR_LOG( "Empty " << label << " catalog field at line " << lineNumber << " in '" << catalogs.back() << "'." )
                continue;
            }

            if ( !addRecord( fields ) ) {
                ERROR_LOG( "Rejected " << label << " catalog entry at line " << lineNumber << " in '" << catalogs.back() << "'." )
            }
        }
    }

    std::vector<MapEventTranslationRecord> loadJapaneseMapRumorTranslations()
    {
        std::vector<MapEventTranslationRecord> records;

        readCatalog<5>( "ja.map-rumor-translation.tsv", "Map rumor translation", [&records]( std::array<std::string, 5> & fields ) {
            const bool duplicate = std::any_of( records.cbegin(), records.cend(), [&fields]( const MapEventTranslationRecord & record ) {
                return record.fileName == fields[0] && record.sourceName == fields[1] && record.sourceDescription == fields[2] && record.sourceEventText == fields[3];
            } );
            if ( duplicate ) {
                return false;
            }

            records.push_back( { std::move( fields[0] ), std::move( fields[1] ), std::move( fields[2] ), std::move( fields[3] ), std::move( fields[4] ) } );
            return true;
        } );

        return records;
    }

    std::vector<MapSphinxTranslationRecord> loadJapaneseMapSphinxTranslations()
    {
        std::vector<MapSphinxTranslationRecord> records;

        readCatalog<6>( "ja.map-sphinx-translation.tsv", "Map sphinx translation", [&records]( std::array<std::string, 6> & fields ) {
            if ( fields[3] != "sphinx-riddle" && fields[3] != "sphinx-answer" ) {
                return false;
            }

            const bool isAnswer = fields[3] == "sphinx-answer";

            // A riddle has exactly one translation, while an answer may have several additional answers.
            const bool duplicate = std::any_of( records.cbegin(), records.cend(), [&fields, isAnswer]( const MapSphinxTranslationRecord & record ) {
                return record.fileName == fields[0] && record.sourceName == fields[1] && record.sourceDescription == fields[2] && record.isAnswer == isAnswer
                       && record.sourceText == fields[4] && ( !isAnswer || record.translatedText == fields[5] );
            } );
            if ( duplicate ) {
                return false;
            }

            records.push_back( { std::move( fields[0] ), std::move( fields[1] ), std::move( fields[2] ), isAnswer, std::move( fields[4] ), std::move( fields[5] ) } );
            return true;
        } );

        return records;
    }

    // Unifies full-width and half-width ASCII characters, removes leading and trailing whitespace and lowers the case.
    std::string normalizeAnswerForComparison( const std::string & text )
    {
        std::string result;
        result.reserve( text.size() );

        for ( size_t i = 0; i < text.size(); ) {
            const auto byteAt = [&text, i]( const size_t offset ) { return static_cast<uint32_t>( static_cast<uint8_t>( text[i + offset] ) ); };

            if ( i + 2 < text.size() && ( byteAt( 0 ) & 0xF0 ) == 0xE0 && ( byteAt( 1 ) & 0xC0 ) == 0x80 && ( byteAt( 2 ) & 0xC0 ) == 0x80 ) {
                const uint32_t code = ( ( byteAt( 0 ) & 0x0F ) << 12 ) | ( ( byteAt( 1 ) & 0x3F ) << 6 ) | ( byteAt( 2 ) & 0x3F );

                if ( code == 0x3000 ) {
                    result += ' ';
                    i += 3;
                    continue;
                }
                if ( code >= 0xFF01 && code <= 0xFF5E ) {
                    result += static_cast<char>( code - 0xFF01 + 0x21 );
                    i += 3;
                    continue;
                }
            }

            result += text[i];
            ++i;
        }

        const auto isWhitespace = []( const char ch ) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; };
        const size_t begin = static_cast<size_t>( std::find_if_not( result.cbegin(), result.cend(), isWhitespace ) - result.cbegin() );
        if ( begin == result.size() ) {
            return {};
        }

        const size_t end = result.size() - static_cast<size_t>( std::find_if_not( result.crbegin(), result.crend(), isWhitespace ) - result.crbegin() );
        result = result.substr( begin, end - begin );

        for ( char & ch : result ) {
            if ( ch >= 'A' && ch <= 'Z' ) {
                ch = static_cast<char>( ch - 'A' + 'a' );
            }
        }

        return result;
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

Maps::MapRumorTextForDisplay Maps::resolveMapRumorTextForDisplay( const FileInfo & info, const std::string & sourceRumorText )
{
    if ( info.filename.empty() || info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return { sourceRumorText, info.getSupportedLanguage() };
    }

    static const std::vector<MapEventTranslationRecord> records = loadJapaneseMapRumorTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    const auto record = std::find_if( records.cbegin(), records.cend(), [&fileName, &info, &sourceRumorText]( const MapEventTranslationRecord & candidate ) {
        return candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description
               && candidate.sourceEventText == sourceRumorText;
    } );
    if ( record != records.cend() ) {
        return { record->translatedEventText, fheroes2::SupportedLanguage::Japanese };
    }

    return { sourceRumorText, info.getSupportedLanguage() };
}

Maps::MapSphinxRiddleForDisplay Maps::resolveMapSphinxRiddleForDisplay( const FileInfo & info, const std::string & sourceRiddleText )
{
    if ( info.filename.empty() || info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return { sourceRiddleText, info.getSupportedLanguage() };
    }

    static const std::vector<MapSphinxTranslationRecord> records = loadJapaneseMapSphinxTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    const auto record = std::find_if( records.cbegin(), records.cend(), [&fileName, &info, &sourceRiddleText]( const MapSphinxTranslationRecord & candidate ) {
        return !candidate.isAnswer && candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description
               && candidate.sourceText == sourceRiddleText;
    } );
    if ( record != records.cend() ) {
        return { record->translatedText, fheroes2::SupportedLanguage::Japanese };
    }

    return { sourceRiddleText, info.getSupportedLanguage() };
}

bool Maps::isAdditionalSphinxAnswerAccepted( const FileInfo & info, const std::list<std::string> & sourceAnswers, const std::string & input )
{
    if ( info.filename.empty() || info.version == GameVersion::RESURRECTION || fheroes2::getCurrentLanguage() != fheroes2::SupportedLanguage::Japanese ) {
        return false;
    }

    const std::string normalizedInput = normalizeAnswerForComparison( input );
    if ( normalizedInput.empty() ) {
        return false;
    }

    static const std::vector<MapSphinxTranslationRecord> records = loadJapaneseMapSphinxTranslations();
    const std::string fileName = System::GetFileName( info.filename );

    return std::any_of( records.cbegin(), records.cend(), [&]( const MapSphinxTranslationRecord & candidate ) {
        return candidate.isAnswer && candidate.fileName == fileName && candidate.sourceName == info.name && candidate.sourceDescription == info.description
               && std::find( sourceAnswers.cbegin(), sourceAnswers.cend(), candidate.sourceText ) != sourceAnswers.cend()
               && normalizeAnswerForComparison( candidate.translatedText ) == normalizedInput;
    } );
}
