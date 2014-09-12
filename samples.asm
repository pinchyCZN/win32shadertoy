;nasm -t -f  win32 -o$(IntDir)\$(InputName).obj -Xvc $(InputName).asm

SECTION .data

%define total_samples 13

%macro inc_sample 1

global _sample%1
_sample%1:
%defstr number %1
%strcat sample_path ".\samples\sample",number,".txt"
	incbin sample_path
	db 0
_endsample%1:

%endmacro

%assign i 1
%rep total_samples

inc_sample i

%assign i i+1
%endrep


global _samples
_samples:

%macro dd_pointer 1
	dd _sample%1
%endmacro

%assign i 1
%rep total_samples
	dd_pointer i
%assign i i+1
%endrep

dd 0,0,0,0

