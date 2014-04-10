;nasmw -t -f  win32 -o$(IntDir)\$(InputName).obj -Xvc $(InputName).asm

SECTION .data

global _tex00jpg
_tex00jpg:
incbin ".\textures\tex00.jpg"
;incbin ".\textures\tex00_512x512_RGB.bin"

global _tex01jpg
_tex01jpg:
incbin ".\textures\tex01.jpg"
;incbin ".\textures\tex01_1024x1024_RGB.bin"

global _tex02jpg
_tex02jpg:
incbin ".\textures\tex02.jpg"
;incbin ".\textures\tex02_512x512_RGB.bin"

global _tex03jpg
_tex03jpg:
incbin ".\textures\tex03.jpg"
;incbin ".\textures\tex03_512x512_RGB.bin"

global _tex04jpg
_tex04jpg:
incbin ".\textures\tex04.jpg"
;incbin ".\textures\tex04_512x512_RGB.bin"

global _tex05jpg
_tex05jpg:
incbin ".\textures\tex05.jpg"
;incbin ".\textures\tex05_1024x1024_RGB.bin"

global _tex06jpg
_tex06jpg:
incbin ".\textures\tex06.jpg"
;incbin ".\textures\tex06_1024x1024_RGB.bin"

global _tex07jpg
_tex07jpg:
incbin ".\textures\tex07.jpg"
;incbin ".\textures\tex07_1024x1024_RGB.bin"

global _tex08jpg
_tex08jpg:
incbin ".\textures\tex08.jpg"
;incbin ".\textures\tex08_512x512_RGB.bin"

global _tex09jpg
_tex09jpg:
incbin ".\textures\tex09.jpg"
;incbin ".\textures\tex09_1024x1024_RGB.bin"

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



