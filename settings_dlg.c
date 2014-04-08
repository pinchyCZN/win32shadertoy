#include <windows.h>
#include "resource.h"

extern int load_preamble;
extern int fragid,progid;
extern int src_sample;
extern const char *sample1,*sample2,*sample3;
extern HINSTANCE ghinstance;
extern HWND heditwin;

struct FONT_NAME{
	int font_num;
	char *font_name;
};
struct FONT_NAME font_names[7]={
	{OEM_FIXED_FONT,"OEM_FIXED_FONT"},
	{ANSI_FIXED_FONT,"ANSI_FIXED_FONT"},
	{ANSI_VAR_FONT,"ANSI_VAR_FONT"},
	{SYSTEM_FONT,"SYSTEM_FONT"},
	{DEVICE_DEFAULT_FONT,"DEVICE_DEFAULT_FONT"},
	{SYSTEM_FIXED_FONT,"SYSTEM_FIXED_FONT"},
	{DEFAULT_GUI_FONT,"DEFAULT_GUI_FONT"}
};
int fontname_to_int(char *name)
{
	int i;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++){
		if(stricmp(name,font_names[i].font_name)==0){
			return font_names[i].font_num;
		}
	}
	return DEFAULT_GUI_FONT;
}

LRESULT CALLBACK texture_select(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HBITMAP hbit=0;
	static char data[64*64*3];
	switch(msg){
	case WM_INITDIALOG:
		if(hbit==0){
			{
				int i;
				for(i=0;i<64*64*3;i++)
					data[i]=rand();
			}
			hbit=CreateBitmap(64,64,1,24,data);
		}
		break;
	case WM_DRAWITEM:
		{
		LPDRAWITEMSTRUCT lpDIS=lparam;
		HDC hDC=lpDIS->hDC;
		RECT rect=lpDIS->rcItem;
	
		if(hbit){
			RECT rcImage;
			BITMAP bm;
			HDC hdc;
			HBITMAP hold;
			LONG cxBitmap, cyBitmap;
			LONG image_width,image_height;
			if ( GetObject(hbit, sizeof(bm), &bm) ) {
				cxBitmap = bm.bmWidth;
				cyBitmap = bm.bmHeight;
			}
			
			// Center image horizontally  
			CopyRect(&rcImage, &rect);
			image_width = rcImage.right - rcImage.left;
			image_height = rcImage.bottom - rcImage.top;
			rcImage.left = (image_width - cxBitmap)/2;
			rcImage.top = (image_height - cyBitmap)/2;
			hdc=CreateCompatibleDC(lpDIS->hDC);
			hold=SelectObject(hdc,hbit);
			StretchBlt(
				lpDIS->hDC, // destination DC 
				lpDIS->rcItem.left, // x upper left 
				lpDIS->rcItem.top, // y upper left 
				lpDIS->rcItem.right - lpDIS->rcItem.left, 
				lpDIS->rcItem.bottom - lpDIS->rcItem.top, 
				hdc, // source device context 
				0, 0, // x and y upper left 
				64, // source bitmap width 
				64, // source bitmap height 
				SRCCOPY); // raster operation

			SelectObject(hdc,hold);
			if(hdc)
				 DeleteDC(hdc);
		}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
		break;
	}
	return 0;
}

LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hedit=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			int i;
			HFONT hfont;
			LOGFONT lf={0};
			hedit=lparam;
			for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++)
				SendDlgItemMessage(hwnd,IDC_FONTS,CB_ADDSTRING,0,font_names[i].font_name);
			hfont=SendMessage(hedit,WM_GETFONT,0,0);
			GetObject(hfont,sizeof(LOGFONT),&lf);
			i=SendDlgItemMessage(hwnd,IDC_FONTS,CB_ADDSTRING,0,lf.lfFaceName);
			SendDlgItemMessage(hwnd,IDC_FONTS,CB_SETCURSEL,i,0);
			if(load_preamble)
				SendDlgItemMessage(hwnd,IDC_LOAD_PREAMBLE,BM_SETCHECK,BST_CHECKED,0);
			{
				char str[40];
				sprintf(str,"load sample %i",src_sample);
				SetDlgItemText(hwnd,IDC_LOAD_SAMPLE,str);
			}

		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_FONTS:
			if(HIWORD(wparam)==CBN_SELENDOK){
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_FONTS,str,sizeof(str));
				font=fontname_to_int(str);
				SendDlgItemMessage(heditwin,IDC_EDIT1,WM_SETFONT,GetStockObject(font),0);
				InvalidateRect(GetDlgItem(heditwin,IDC_EDIT1),NULL,TRUE);
			}
			break;
		case IDC_LOAD_PREAMBLE:
			if(BST_CHECKED==SendMessage(lparam,BM_GETCHECK,0,0))
				load_preamble=TRUE;
			else
				load_preamble=FALSE;
			break;
		case IDC_LOAD_SAMPLE:
			if(HIWORD(wparam)==BN_CLICKED){
				char *str=sample1;
				switch(src_sample){
					default:case 1:str=sample1;break;
					case 2:str=sample2;break;
					case 3:str=sample3;break;
				}
				load_shader_string(fragid,str,GetDlgItem(heditwin,IDC_EDIT1));
				compile(heditwin);
				src_sample++;
				if(src_sample>3)
					src_sample=1;
				{
					char tmp[40];
					sprintf(tmp,"load sample %i",src_sample);
					SetDlgItemText(hwnd,IDC_LOAD_SAMPLE,tmp);
				}
			}
			break;
		case IDC_CHAN1:
		case IDC_CHAN2:
		case IDC_CHAN3:
		case IDC_CHAN4:
			if(HIWORD(wparam)==BN_CLICKED){
				DialogBox(ghinstance,MAKEINTRESOURCE(IDD_TEXTURES),hwnd,texture_select);
			}
			break;

		case IDCANCEL:
		case IDOK:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd,0);
	}
	return 0;
}