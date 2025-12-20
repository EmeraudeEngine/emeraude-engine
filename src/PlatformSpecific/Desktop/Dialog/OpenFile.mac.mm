#include "OpenFile.hpp"

#if IS_MACOS

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
            NSString * title = [NSString stringWithCString:this->title().c_str()
            encoding:[NSString defaultCStringEncoding]];

            NSOpenPanel * panel = [NSOpenPanel openPanel];

            [panel setTitle:title];
            [panel setCanChooseFiles:YES];
            [panel setCanChooseDirectories:NO];
            [panel setAllowsMultipleSelection:YES];

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

#endif
