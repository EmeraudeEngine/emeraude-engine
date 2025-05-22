#include "SaveFile.hpp"

#if IS_MACOS

/* Third-party inclusions. */
#import <AppKit/AppKit.h>

/* Local inclusions. */
#include "Window.hpp"

namespace EmEn::PlatformSpecific::Desktop::Dialog
{
	bool
	SaveFile::execute (Window * /*window*/) noexcept
	{
		@autoreleasepool
        {
            [NSApplication sharedApplication]; // FIXME: Check the usefulness of this line !
            [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

            NSString * title = [NSString stringWithCString:this->title().c_str()
            encoding:[NSString defaultCStringEncoding]];

            NSSavePanel * panel = [NSSavePanel savePanel];

            [panel setTitle:title];

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

            // Passing empty array to setAllowedFileTypes will cause exception.
            NSArray * file_types = nil;
            NSUInteger count = [file_types_list count];

            if ( count > 0 )
            {
                file_types = [[file_types_list objectAtIndex:0] allObjects];

                // If we meet a '*' file extension, we allow all the file types and no
                // need to set the specified file types.
                if ( [file_types count] == 0 || [file_types containsObject:@"*"] )
                {
                    file_types = nil;
                }
            }

			[panel setAllowedFileTypes:file_types];
			//panel.allowedContentTypes = file_types; // Recommanded new way

            // TODO: Format picker not yet implemented, macOS doesnt support it natively.
            // To create it like electron see : https://github.com/electron/electron/blob/main/shell/browser/ui/file_dialog_mac.mm#L133
            [panel setLevel:CGShieldingWindowLevel()];
            NSInteger result = [panel runModal];

            if ( result == NSModalResponseOK )
            {
                NSURL * url = [panel URL];
                NSString * filepath = [url path];

				m_filepath = [filepath UTF8String];
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
