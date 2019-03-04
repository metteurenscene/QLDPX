# QLDPX

A simple macOS QuickLook plugin to generate thumbnails and previews of DPX files.

## Installation

Copy `QLDPX.qlgenerator` into `~/Library/QuickLook` or `/Library/QuickLook`.

## Content Type UTI

It is likely that no [UTI](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/understanding_utis/understand_utis_intro/understand_utis_intro.html) has been declared for DPX files on your system. To work around this, the `Document Content Type UTIs` entry in `Info.plist` is set to `public.item` and the given file's extension is checked for `.dpx`. As a result, the QLDPX generator may be called more often than is necessary. If you want to avoid this, replace `public.item` with the UTI for DPX files on your system and rebuild.

See here how to [check a file's UTI](https://superuser.com/questions/209145/how-to-get-a-files-uti-from-the-command-line-in-mac-os-x).
