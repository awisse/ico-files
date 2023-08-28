# Experimentation with Windows .ico files

I noticed that [LibreSprite](https://github.com/LibreSprite/LibreSprite) and [Aseprite](https://github.com/aseprite/aseprite) 
only read .ico files correctly if the image is in 24-bit color depth. 2-, 4-, 8- and 32-bit color depths are not handled.
In order to better understand this file format, I looked up some documentation on Wikipedia
[ICO (file format)](https://en.wikipedia.org/wiki/ICO_(file_format)) and [BMP](https://en.wikipedia.org/wiki/BMP_file_format), 
as well as Microsoft Learn: [Icons](https://learn.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)),
[BITMAPINFOHEADER](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader) 
and [The evolution of the ICO file format](https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513).

Libresprite also doesn't import bitmaps (.bmp) with 2-bit and 4-bit palettes correctly.

I wrote this code as an exercice to understand the position of the metadata and imagedata of these formats.

Looking into working on LibreSprite one day to better import these formats.
