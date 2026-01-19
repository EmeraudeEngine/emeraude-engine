/*
 * src/PlatformSpecific/Desktop/Dialog/OpenFile.mac.mm
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "OpenFile.hpp"

/* STL inclusions. */
#include <filesystem>

/* Third-party inclusions. */
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	OpenFile::execute (Window * /*window*/) noexcept
	{
        @autoreleasepool
        {
            NSString * title = [NSString stringWithUTF8String:this->title().c_str()];

            NSOpenPanel * panel = [NSOpenPanel openPanel];

            [panel setTitle:title];

            /* Configure file/folder selection mode. */
            if ( m_selectFolder )
            {
                [panel setCanChooseFiles:NO];
                [panel setCanChooseDirectories:YES];
            }
            else
            {
                [panel setCanChooseFiles:YES];
                [panel setCanChooseDirectories:NO];
            }

            [panel setAllowsMultipleSelection:m_multiSelect ? YES : NO];

            /* Apply file extension filters only when selecting files, not folders. */
            if ( !m_selectFolder && !m_extensionFilters.empty() )
            {
                NSMutableArray * file_types_list = [NSMutableArray array];
                NSMutableArray * filter_names = [NSMutableArray array];
                NSMutableOrderedSet * file_type_set = [NSMutableOrderedSet orderedSetWithCapacity:m_extensionFilters.size()];

                for ( auto & filter : m_extensionFilters )
                {
                    [filter_names addObject:@(filter.first.c_str())];

                    for ( std::string & ext : filter.second )
                    {
                        auto pos = ext.rfind('.');

                        if ( pos != std::string::npos )
                        {
                            ext.erase(0, pos + 1);
                        }

                        [file_type_set addObject:@(ext.c_str())];
                    }
                }
                [file_types_list addObject:[file_type_set array]];

                // Convert extensions to UTTypes for allowedContentTypes (macOS 12.0+)
                NSMutableArray<UTType *> * contentTypes = [NSMutableArray array];
                NSUInteger count = [file_types_list count];

                if ( count > 0 )
                {
                    NSArray * extensions = [[file_types_list objectAtIndex:0] allObjects];

                    // If we meet a '*' file extension, we allow all the file types
                    if ( [extensions count] > 0 && ![extensions containsObject:@"*"] )
                    {
                        for ( NSString * ext in extensions )
                        {
                            UTType * type = [UTType typeWithFilenameExtension:ext];

                            if ( type != nil )
                            {
                                [contentTypes addObject:type];
                            }
                        }
                    }
                }

                if ( [contentTypes count] > 0 )
                {
                    panel.allowedContentTypes = contentTypes;
                }
            }

            // TODO: Format picker not yet implemented, macOS doesnt support it natively.
            // To create it like electron see : https://github.com/electron/electron/blob/main/shell/browser/ui/file_dialog_mac.mm#L133
            [panel setLevel:CGShieldingWindowLevel()];
            NSInteger result = [panel runModal];

            if ( result == NSModalResponseOK )
            {
                NSArray * URLs = [panel URLs];

                for ( NSURL * url in URLs )
                {
                    NSString * filepath = [url path];

                    m_filepaths.emplace_back([filepath UTF8String]);
                }
            }
            else
            {
                NSLog(@"File picker canceled");
            }
        }  // end of autorelease pool.

        return true;
	}
}
