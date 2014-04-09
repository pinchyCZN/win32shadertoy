;nasmw -t -f  win32 -o$(IntDir)\$(InputName).obj -Xvc $(InputName).asm

SECTION .data

global _tex00_512x512_RGB
_tex00_512x512_RGB:
incbin ".\textures\tex00_512x512_RGB.bin"

global _tex01_1024x1024_RGB
_tex01_1024x1024_RGB:
incbin ".\textures\tex01_1024x1024_RGB.bin"

global _tex02_512x512_RGB
_tex02_512x512_RGB:
incbin ".\textures\tex02_512x512_RGB.bin"

global _tex03_512x512_RGB
_tex03_512x512_RGB:
incbin ".\textures\tex03_512x512_RGB.bin"

global _tex04_512x512_RGB
_tex04_512x512_RGB:
incbin ".\textures\tex04_512x512_RGB.bin"

global _tex11_64x64_RGBA
_tex11_64x64_RGBA:
incbin ".\textures\tex11_64x64_RGBA.bin"

