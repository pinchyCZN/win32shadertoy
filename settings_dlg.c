#include <windows.h>
#include "resource.h"

extern int load_preamble;
extern int fragid,progid;
extern int src_sample;
extern const char *sample1,*sample2,*sample3;
extern HINSTANCE ghinstance;
extern HWND heditwin;
extern unsigned char tex00_512x512_RGB[];
extern unsigned char tex02_512x512_RGB[];
extern unsigned char tex11_64x64_RGBA[];


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
	static char data[64*64*3];
	switch(msg){
	case WM_INITDIALOG:
		{
			int i,j,id=IDC_BUTTON1+1;
			int ycaption=GetSystemMetrics(SM_CYCAPTION);
			for(i=0;i<4;i++){
				for(j=0;j<4;j++){
					HWND h;
					h=CreateWindow("BUTTON","BUTTON",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_CHECKBOX|BS_PUSHLIKE,4+i*(64+4),4+j*(64+4),64,64,hwnd,id,ghinstance,NULL);
					id++;
				}
			}
			ShowWindow(GetDlgItem(hwnd,IDC_BUTTON1),SW_HIDE);
			SetWindowPos(hwnd,NULL,0,0,4*2+(64+4)*4,ycaption+4*2+(64+4)*4,SWP_NOMOVE|SWP_NOZORDER);
		}
		break;
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDIS=lparam;
			BITMAPINFO bmi;
			HDC hdc;
			int flags;
			memset(&bmi,0,sizeof(bmi));
			bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biBitCount=24;
			bmi.bmiHeader.biHeight=64;
			bmi.bmiHeader.biWidth=64;
			bmi.bmiHeader.biPlanes=1;
			bmi.bmiHeader.biCompression=BI_RGB;
			bmi.bmiHeader.biSizeImage = ((bmi.bmiHeader.biWidth * 32 +31)& ~31) /8 * bmi.bmiHeader.biHeight;
		{
			int x,y;
			for(x=0;x<64;x++){
				for(y=0;y<64;y++){
					int delta;
					for(delta=0;delta<3;delta++){
						int i,j,c=0;
						int flip;
						for(i=0;i<8;i++){
							for(j=0;j<8;j++){
								c+=tex02_512x512_RGB[2-delta+i*3+j*3*512+x*8*3+y*3*512*8];
							}
						}
						flip=(64*3*63);
						data[delta+x*3+flip-y*64*3]=c/64; //tex00_512x512_RGB[index+(2-p)];
					}
				}
			}
		}
			hdc=lpDIS->hDC;
			SetDIBitsToDevice(
				hdc,
				lpDIS->rcItem.left,
				lpDIS->rcItem.top,
				lpDIS->rcItem.right - lpDIS->rcItem.left,
				lpDIS->rcItem.bottom - lpDIS->rcItem.top,
				0,0,
				0,64,
				data,
				&bmi,
				DIB_RGB_COLORS);
			if(lpDIS->itemState&ODS_SELECTED)
				flags=EDGE_SUNKEN;
			else
				flags=EDGE_RAISED;
			DrawEdge(hdc,&lpDIS->rcItem,flags,BF_RECT);

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