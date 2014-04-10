#include <windows.h>
#include <GL/gl.h>
#include "glext.h"

#include "resource.h"

extern int load_preamble;
extern int fragid,progid;
extern int src_sample;
extern const char *sample1,*sample2,*sample3;
extern HINSTANCE ghinstance;
extern HWND heditwin;
extern unsigned char tex00_512x512_RGB[];
extern unsigned char tex01_1024x1024_RGB[];
extern unsigned char tex02_512x512_RGB[];
extern unsigned char tex03_512x512_RGB[];
extern unsigned char tex04_512x512_RGB[];
extern unsigned char tex05_1024x1024_RGB[];
extern unsigned char tex06_1024x1024_RGB[];
extern unsigned char tex07_1024x1024_RGB[];
extern unsigned char tex08_512x512_RGB[];
extern unsigned char tex09_1024x1024_RGB[];
extern unsigned char tex10_64x64_L[];
extern unsigned char tex11_64x64_RGBA[];
extern unsigned char tex12_256x256_L[];
extern unsigned char tex14_256x32_RGBA[];
extern unsigned char tex15_8x8_L[];
extern unsigned char tex16_256x256_RGBA[];

static PFNGLCREATESHADERPROC glActiveTexture=0;

struct TEXTURE_FILE{
	char *name;
	char *data;
	int w,h;
	int bpp;
};
struct TEX_BUTTON{
	HWND hwnd;
	int id;
	int pressed;
	char *thumb;
	void *tfile;
};
struct GL_TEXTURE_INFO{
	int gltexture;
	void *tex_button;
};
//int gltexture1=0,gltexture2=0,gltexture3=0,gltexture4=0;
struct GL_TEXTURE_INFO gl_textures[4]={0};

#define NUM_TEXTURES 16
struct TEXTURE_FILE tex_files[]={
	{"tex00_512x512_RGB",tex00_512x512_RGB,512,512,3},
	{"tex01_1024x1024_RGB",tex01_1024x1024_RGB,1024,1024,3},
	{"tex02_512x512_RGB",tex02_512x512_RGB,512,512,3},
	{"tex03_512x512_RGB",tex03_512x512_RGB,512,512,3},
	{"tex04_512x512_RGB",tex04_512x512_RGB,512,512,3},
	{"tex05_1024x1024_RGB",tex05_1024x1024_RGB,1024,1024,3},
	{"tex06_1024x1024_RGB",tex06_1024x1024_RGB,1024,1024,3},
	{"tex07_1024x1024_RGB",tex07_1024x1024_RGB,1024,1024,3},
	{"tex08_512x512_RGB",tex08_512x512_RGB,512,512,3},
	{"tex09_1024x1024_RGB",tex09_1024x1024_RGB,1024,1024,3},
	{"tex10_64x64_L",tex10_64x64_L,64,64,1},
	{"tex11_64x64_RGBA",tex11_64x64_RGBA,64,64,4},
	{"tex12_256x256_L",tex12_256x256_L,256,256,1},
	{"tex14_256x32_RGBA",tex14_256x32_RGBA,256,32,4},
	{"tex15_8x8_L",tex15_8x8_L,8,8,1},
	{"tex16_256x256_RGBA",tex16_256x256_RGBA,256,256,4}
};
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
//outbuf is BGR format 3bpp
int downsample(unsigned char *inbuf,int inw,int inh,int bpp,unsigned char *outbuf,int outw,int outh)
{
	int x,y;
	int flip,modx=1,mody=1;
	if(outw!=0 && outw<inw)
		modx=inw/outw;
	if(outh!=0 && outh<inh)
		mody=inh/outh;
	flip=outw*3*(outh-1);
	for(x=0;x<outw;x++){
		for(y=0;y<outh;y++){
			int delta;
			for(delta=0;delta<3;delta++){
				int i,j,c=0;
				int offset=(bpp-1)-delta;
				if(bpp==1)
					offset=0;
				else if(bpp==4)
					offset--;
				if(x<inw && y<inh){
					for(i=0;i<modx;i++){
						for(j=0;j<mody;j++){
							c+=inbuf[offset +i*bpp +j*bpp*inw +x*modx*bpp +y*bpp*inw*mody];
						}
					}
				}
				if(modx!=0 && mody!=0)
					c/=modx*mody;
				outbuf[delta+x*3+flip-y*outw*3]=c;
			}
		}
	}
	return TRUE;
}
			/*
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
			*/

int bind_textures(struct GL_TEXTURE_INFO *textures)
{
	int i;
	if(textures==0)
		return FALSE;
	for(i=0;i<4;i++){
		int *tn=0;
		tn=&textures[i].gltexture;
		if(*tn==0)
			glGenTextures(1,tn);

		if(*tn!=0){
			struct TEX_BUTTON *tb;
			int w=512,h=512;
			int format=GL_RGB,colors=3;
			char *data=tex00_512x512_RGB;
			tb=textures[i].tex_button;
			if(tb && tb->tfile){
				struct TEXTURE_FILE *tf=tb->tfile;
				if(tf){
					if(tf->data)
						data=tf->data;
					if(tf->w && tf->h){
						w=tf->w;
						h=tf->h;
					}
					if(tf->bpp){
						switch(tf->bpp){
						case 1:format=GL_LUMINANCE;colors=1;break;
						case 3:format=GL_RGB;colors=3;break;
						case 4:format=GL_RGBA;colors=4;break;
						}
					}
				}
			}
			if(glActiveTexture==0)
				glActiveTexture=wglGetProcAddress("glActiveTexture");
			if(glActiveTexture)
				glActiveTexture(GL_TEXTURE0+i);
			glBindTexture(GL_TEXTURE_2D,*tn);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
			glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
			glTexImage2D(GL_TEXTURE_2D,0,colors,w,h,0,format,GL_UNSIGNED_BYTE,data);
		}
	}
	return TRUE;
}
int load_textures()
{
	return bind_textures(&gl_textures);
}
LRESULT CALLBACK texture_select(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static struct TEX_BUTTON buttons[NUM_TEXTURES];
	static int load_textures=TRUE;
	static char *title=0;
	static int current_channel=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			int i,j,id=IDC_BUTTON1+1;
			int index=0;
			int ycaption=GetSystemMetrics(SM_CYCAPTION);
			for(j=0;j<NUM_TEXTURES/4;j++){
				for(i=0;i<NUM_TEXTURES/4;i++){
					HWND h;
					h=CreateWindow("BUTTON","BUTTON",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_PUSHBUTTON,4+i*(64+4),4+j*(64+4),64,64,hwnd,id,ghinstance,NULL);
					buttons[index].hwnd=h;
					buttons[index].id=id;
					buttons[index].pressed=FALSE;
					index++;
					id++;
					//h=CreateWindow("TEXT","BUTTON",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|BS_PUSHBUTTON,4+i*(64+4),4+j*(64+4),64,64,hwnd,id,ghinstance,NULL);
				}
			}
			if(load_textures){
				load_textures=FALSE;
				for(i=0;i<sizeof(tex_files)/sizeof(struct TEXTURE_FILE);i++){
					buttons[i].tfile=&tex_files[i];
					buttons[i].thumb=malloc(64*64*3);
					if(buttons[i].thumb){
						downsample(tex_files[i].data,tex_files[i].w,tex_files[i].h,tex_files[i].bpp,buttons[i].thumb,64,64);
					}
				}
			}
			ShowWindow(GetDlgItem(hwnd,IDC_BUTTON1),SW_HIDE);
			SetWindowPos(hwnd,NULL,0,0,4*2+(64+4)*4,ycaption+4*2+(64+4)*4,SWP_NOMOVE|SWP_NOZORDER);
			title=lparam;
			if(title){
				struct TEX_BUTTON *tb=gl_textures[current_channel].tex_button;
				for(i=0;i<4;i++){
					if(strchr(title,'0'+i))
						current_channel=i;
				}
				if(tb && tb->tfile){
					char str[80];
					struct TEXTURE_FILE *tf=tb->tfile;
					if(tf->name){
						_snprintf(str,sizeof(str),"%s - %s",title,tf->name);
						str[sizeof(str)-1]=0;
						SetWindowText(hwnd,str);
					}
				}
				else
					SetWindowText(hwnd,title);
			}
			for(i=0;i<NUM_TEXTURES;i++){
				if(gl_textures[current_channel].tex_button){
					struct TEX_BUTTON *tb=gl_textures[current_channel].tex_button;
					tb->pressed=TRUE;
				}
			}
		}
		break;
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDIS=lparam;
			BITMAPINFO bmi;
			HDC hdc;
			int i,flags,selected=FALSE;
			char *texbuf=0;
			memset(&bmi,0,sizeof(bmi));
			bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biBitCount=24;
			bmi.bmiHeader.biHeight=64;
			bmi.bmiHeader.biWidth=64;
			bmi.bmiHeader.biPlanes=1;
			bmi.bmiHeader.biCompression=BI_RGB;
			bmi.bmiHeader.biSizeImage = ((bmi.bmiHeader.biWidth * 32 +31)& ~31) /8 * bmi.bmiHeader.biHeight;

			hdc=lpDIS->hDC;
			for(i=0;i<NUM_TEXTURES;i++){
				if(buttons[i].id==lpDIS->CtlID){
					texbuf=buttons[i].thumb;
					selected=buttons[i].pressed;
				}
			}
			SetDIBitsToDevice(
				hdc,
				lpDIS->rcItem.left,
				lpDIS->rcItem.top,
				lpDIS->rcItem.right - lpDIS->rcItem.left,
				lpDIS->rcItem.bottom - lpDIS->rcItem.top,
				0,0,
				0,64,
				texbuf,
				&bmi,
				DIB_RGB_COLORS);
			/*
			if(selected)
				flags=EDGE_SUNKEN;
			else
				flags=EDGE_RAISED;
			DrawEdge(hdc,&lpDIS->rcItem,flags,BF_RECT);
			*/
			if(selected){
				static HBRUSH red=0;
				if(red==0)
					red=CreateSolidBrush(0xFF);
				FrameRect(hdc,&lpDIS->rcItem,red);
			}

		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		default:
			if(HIWORD(wparam)!=BN_CLICKED)
				break;
			if(LOWORD(wparam)>=IDC_BUTTON1+1){
				int id=LOWORD(wparam);
				int i;
				for(i=0;i<NUM_TEXTURES;i++){
					if(id==buttons[i].id){
						struct TEXTURE_FILE *tf;
						tf=buttons[i].tfile;
						if(tf){
							char str[80];
							_snprintf(str,sizeof(str),"%s - %s",title,tf->name);
							str[sizeof(str)-1]=0;
							SetWindowText(hwnd,str);
						}
						buttons[i].pressed=TRUE;
						InvalidateRect(buttons[i].hwnd,0,FALSE);
						gl_textures[current_channel].tex_button=&buttons[i];
					}
					else{
						int inval=FALSE;
						if(buttons[i].pressed)
							inval=TRUE;
						buttons[i].pressed=FALSE;
						if(inval)
							InvalidateRect(buttons[i].hwnd,0,FALSE);
					}
				}
				bind_textures(&gl_textures);
			}
			break;
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
		case IDC_CHAN0:
		case IDC_CHAN1:
		case IDC_CHAN2:
		case IDC_CHAN3:
			if(HIWORD(wparam)==BN_CLICKED){
				static char str[40]={0};
				GetWindowText(lparam,str,sizeof(str));
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_TEXTURES),hwnd,texture_select,str);
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