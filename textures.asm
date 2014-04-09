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

global _tex05_1024x1024_RGB
_tex05_1024x1024_RGB:
incbin ".\textures\tex05_1024x1024_RGB.bin"

global _tex06_1024x1024_RGB
_tex06_1024x1024_RGB:
incbin ".\textures\tex06_1024x1024_RGB.bin"

global _tex07_1024x1024_RGB
_tex07_1024x1024_RGB:
incbin ".\textures\tex07_1024x1024_RGB.bin"

global _tex08_512x512_RGB
_tex08_512x512_RGB:
incbin ".\textures\tex08_512x512_RGB.bin"

global _tex09_1024x1024_RGB
_tex09_1024x1024_RGB:
incbin ".\textures\tex09_1024x1024_RGB.bin"

global _tex10_64x64_L
_tex10_64x64_L:
incbin ".\textures\tex10_64x64_L.bin"

global _tex11_64x64_RGBA
_tex11_64x64_RGBA:
incbin ".\textures\tex11_64x64_RGBA.bin"

global _tex12_256x256_L
_tex12_256x256_L:
incbin ".\textures\tex12_256x256_L.bin"

global _tex14_256x32_RGBA
_tex14_256x32_RGBA:
incbin ".\textures\tex14_256x32_RGBA.bin"

global _tex15_8x8_L
_tex15_8x8_L:
incbin ".\textures\tex15_8x8_L.bin"

global _tex16_256x256_RGBA
_tex16_256x256_RGBA:
incbin ".\textures\tex16_256x256_RGBA.bin"



