#include "file_picker.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>

namespace minidfs::core {

FilePickerResult FilePicker::show_dialog(const FilePickerOptions& options) {
    FilePickerResult result;
    result.success = false;

    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];

        // Configure the panel
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:YES];
        [panel setTitle:[NSString stringWithUTF8String:options.title.c_str()]];

        // Set default directory if provided
        if (!options.default_directory.empty()) {
            NSString* dirPath = [NSString stringWithUTF8String:options.default_directory.c_str()];
            NSURL* dirURL = [NSURL fileURLWithPath:dirPath];
            [panel setDirectoryURL:dirURL];
        }

        // Set allowed file types if provided
        if (!options.allowed_extensions.empty()) {
            NSMutableArray* types = [NSMutableArray array];
            for (const auto& ext : options.allowed_extensions) {
                [types addObject:[NSString stringWithUTF8String:ext.c_str()]];
            }
            [panel setAllowedFileTypes:types];
        }

        // Show hidden files if requested
        [panel setShowsHiddenFiles:options.show_hidden_files ? YES : NO];

        // Run the panel
        NSModalResponse response = [panel runModal];

        if (response == NSModalResponseOK) {
            result.success = true;
            NSArray* urls = [panel URLs];
            for (NSURL* url in urls) {
                result.paths.push_back([[url path] UTF8String]);
            }
        }
    }

    return result;
}

}

#endif
