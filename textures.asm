;nasmw -t -f  win32 -o$(IntDir)\$(InputName).obj -Xvc "$(InputPath)"
;$(TargetDir)\$(InputName).obj

SECTION .data

global _tex00jpg
_tex00jpg:
incbin ".\textures\tex00.jpg"
;incbin ".\textures\untitled.jpg"

global _tex01jpg
_tex01jpg:
incbin ".\textures\_tex01.jpg"

global _tex02jpg
_tex02jpg:
incbin ".\textures\tex02.jpg"

global _tex03jpg
_tex03jpg:
incbin ".\textures\tex03.jpg"

global _tex04jpg
_tex04jpg:
incbin ".\textures\tex04.jpg"

global _tex05jpg
_tex05jpg:
incbin ".\textures\_tex05.jpg"

global _tex06jpg
_tex06jpg:
incbin ".\textures\_tex06.jpg"

global _tex07jpg
_tex07jpg:
incbin ".\textures\_tex07.jpg"

global _tex08jpg
_tex08jpg:
incbin ".\textures\_tex08.jpg"

global _tex09jpg
_tex09jpg:
incbin ".\textures\_tex09.jpg"

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



