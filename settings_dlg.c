#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include "glext.h"
#include "tjpgd.h"

#include "resource.h"

extern char *samples[];
extern const char sample1[];

extern int load_preamble,use_new_format,compile_on_modify;
extern int fragid,progid;
extern int src_sample;
extern HINSTANCE ghinstance;
extern HWND heditwin;
extern unsigned char tex00jpg[];
extern unsigned char tex01jpg[];
extern unsigned char tex02jpg[];
extern unsigned char tex03jpg[];
extern unsigned char tex04jpg[];
extern unsigned char tex05jpg[];
extern unsigned char tex06jpg[];
extern unsigned char tex07jpg[];
extern unsigned char tex08jpg[];
extern unsigned char tex09jpg[];

unsigned char *tex00_512x512_RGB=0;
unsigned char *tex01_1024x1024_RGB=0;
unsigned char *tex02_512x512_RGB=0;
unsigned char *tex03_512x512_RGB=0;
unsigned char *tex04_512x512_RGB=0;
unsigned char *tex05_1024x1024_RGB=0;
unsigned char *tex06_1024x1024_RGB=0;
unsigned char *tex07_1024x1024_RGB=0;
unsigned char *tex08_512x512_RGB=0;
unsigned char *tex09_1024x1024_RGB=0;

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
static struct TEX_BUTTON buttons[NUM_TEXTURES];



struct TEXTURE_FILE tex_files[]={
	{"tex00_512x512_RGB",0,512,512,3},
	{"tex01_1024x1024_RGB",0,1024,1024,3},
	{"tex02_512x512_RGB",0,512,512,3},
	{"tex03_512x512_RGB",0,512,512,3},
	{"tex04_512x512_RGB",0,512,512,3},
	{"tex05_1024x1024_RGB",0,1024,1024,3},
	{"tex06_1024x1024_RGB",0,1024,1024,3},
	{"tex07_1024x1024_RGB",0,1024,1024,3},
	{"tex08_512x512_RGB",0,512,512,3},
	{"tex09_1024x1024_RGB",0,1024,1024,3},
	{"tex10_64x64_L",tex10_64x64_L,64,64,1},
	{"tex11_64x64_RGBA",tex11_64x64_RGBA,64,64,4},
	{"tex12_256x256_L",tex12_256x256_L,256,256,1},
	{"tex14_256x32_RGBA",tex14_256x32_RGBA,256,32,4},
	{"tex15_8x8_L",tex15_8x8_L,8,8,1},
	{"tex16_256x256_RGBA",tex16_256x256_RGBA,256,256,4}
};
struct DECOMPRESS_LIST{
	unsigned char *src;
	unsigned char **dst;
	struct TEXTURE_FILE *tf;
};
struct DECOMPRESS_LIST jpg_list[]={
	{tex00jpg,&tex00_512x512_RGB,&tex_files[0]},
	{tex01jpg,&tex01_1024x1024_RGB,&tex_files[1]},
	{tex02jpg,&tex02_512x512_RGB,&tex_files[2]},
	{tex03jpg,&tex03_512x512_RGB,&tex_files[3]},
	{tex04jpg,&tex04_512x512_RGB,&tex_files[4]},
	{tex05jpg,&tex05_1024x1024_RGB,&tex_files[5]},
	{tex06jpg,&tex06_1024x1024_RGB,&tex_files[6]},
	{tex07jpg,&tex07_1024x1024_RGB,&tex_files[7]},
	{tex08jpg,&tex08_512x512_RGB,&tex_files[8]},
	{tex09jpg,&tex09_1024x1024_RGB,&tex_files[9]}

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
int get_texture_res(int channel,int *x,int *y)
{
	int result=FALSE;
	if(channel>=0 && channel<4){
		struct TEX_BUTTON *tb=gl_textures[channel].tex_button;
		if(tb){
			struct TEXTURE_FILE *tf=tb->tfile;
			if(tf){
				if(x)
					*x=tf->w;
				if(y)
					*y=tf->h;
				result=TRUE;
			}
		}
	}
	return result;
}
//outbuf is BGR format 3bpp
int downsample(unsigned char *inbuf,int inw,int inh,int bpp,unsigned char *outbuf,int outw,int outh)
{
	int x,y;
	int flip,modx=1,mody=1;
	if(inbuf==0 || outbuf==0)
		return FALSE;
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
struct IOBUF{
	unsigned char *in;
	int position;
	unsigned char *out;
	int outsize;
	int width;
};
UINT in_func (JDEC* jd, BYTE* buff, UINT nbyte)
{
	struct IOBUF *io=jd->device;
	if(io==0 || io->in==0)
		return 0;
	if(buff)
		memcpy(buff,io->in+io->position,nbyte);
	io->position+=nbyte;
	return nbyte;
}
UINT out_func (JDEC* jd, void* bitmap, JRECT* rect)
{
	struct IOBUF *io=jd->device;
	int offset,width;
	if(io==0 || io->out==0)
		return 0;
	offset=3 * (rect->top * io->width + rect->left);
	width=3 * (rect->right - rect->left + 1);
	if(offset+width<=io->outsize){
		int i,height;
		char *src,*dst;
		src=bitmap;
		dst=io->out+offset;
		height=rect->bottom-rect->top+1;
		for(i=0;i<height;i++){
			memcpy(dst+(i*3*io->width),src+(i*width),width);
		}
	}
	else
		printf("decompress size exceeded\n");
	return 1;
}
int init_texture_buttons()
{
	static int load_buttons=TRUE;
	if(load_buttons){
		int i;
		load_buttons=FALSE;
		for(i=0;i<sizeof(tex_files)/sizeof(struct TEXTURE_FILE);i++){
			buttons[i].tfile=&tex_files[i];
			buttons[i].thumb=malloc(64*64*3);
			if(buttons[i].thumb){
				downsample(tex_files[i].data,tex_files[i].w,tex_files[i].h,tex_files[i].bpp,buttons[i].thumb,64,64);
			}
		}
		buttons[0].pressed=TRUE;
		for(i=0;i<4;i++){
			gl_textures[i].tex_button=&buttons[0];
		}
	}
	return load_buttons;
}
int load_textures()
{
	static int unpack_jpg=TRUE;
	if(unpack_jpg){
		void *work;
		int work_size=3100;
		printf("unpacking JPG images\n");
		unpack_jpg=FALSE;
		work=malloc(work_size);
		if(work){
			int i;
			for(i=0;i<sizeof(jpg_list)/sizeof(struct DECOMPRESS_LIST);i++){
				unsigned char *src,*dst;
				JDEC jdec;
				JRESULT res;
				struct IOBUF io;
				src=jpg_list[i].src;
				io.in=src;
				io.position=0;
				res=jd_prepare(&jdec,in_func,work,work_size,&io);
				if(res==JDR_OK){
					int decomp_size=3 * jdec.width * jdec.height;
					printf("Image dimensions: %u by %u. %u bytes used.\n", jdec.width, jdec.height, work_size - jdec.sz_pool);
					dst=malloc(decomp_size);
					if(dst){
						*jpg_list[i].dst=dst;
						io.out=dst;
						io.outsize=decomp_size;
						io.width=jdec.width;
						res=jd_decomp(&jdec,out_func,0);
						if(res==JDR_OK){
							struct TEXTURE_FILE *tf;
							tex_files[i].data=dst;
							tex_files[i].w=jdec.width;
							tex_files[i].h=jdec.height;
							tf=jpg_list[i].tf;
							if(tf){
								tf->data=dst;
							}
							{
								/*
								FILE *f;
								char str[80];
								sprintf(str,"c:\\temp\\test%ix%i.bin",jdec.width,jdec.height);
								f=fopen(str,"wb");
								if(f){
									fwrite(tf->data,1,tf->w*tf->h*3,f);
									fclose(f);
								}
								*/
								
							}
						}
						else
							printf("Failed to decompress: rc=%d\n", res);
					}
				}else{
					struct TEXTURE_FILE *tf;
					tf=jpg_list[i].tf;
					printf("Failed to prepare: rc=%d %s\n", res,tf->name);
				}
			}
			free(work);
		}
	}
	init_texture_buttons();
	return bind_textures(&gl_textures);
}

LRESULT CALLBACK texture_select(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static char *title=0;
	static int current_channel=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			int i,j,id=IDC_BUTTON1+1;
			int index=0;
			int ycaption=GetSystemMetrics(SM_CYCAPTION);
			init_texture_buttons();
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
			int i,selected=FALSE;
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
int toggle_window_size(HWND hwnd)
{
	extern HWND hshaderview;
	RECT rect;
	int i,w,h,max;
	GetWindowRect(hshaderview,&rect);
	w=GetSystemMetrics(SM_CXSCREEN);
	h=GetSystemMetrics(SM_CYSCREEN);
	max=5;
	if(hwnd)
		SetDlgItemText(hwnd,IDC_TINY_WINDOW,"Tiny window");

	for(i=0;i<max;i++){
		int sw,sh;
		sw=w/(2+i);
		sh=h/(2+i);
		if((rect.right-rect.left)>sw){
			SetWindowPos(hshaderview,NULL,0,0,sw,sh,SWP_NOZORDER);
			if(hwnd){
				if(i==(max-1))
					SetDlgItemText(hwnd,IDC_TINY_WINDOW,"smallest");
			}
			break;
		}
		if(i==(max-1)){
			SetWindowPos(hshaderview,NULL,0,0,w/2,h/2,SWP_NOZORDER);
		}

	}
	/*
	if((rect.right-rect.left)>=(w/2)){
		SetWindowPos(hview,NULL,0,0,140,160,SWP_NOZORDER);
	}
	else{
		SetWindowPos(hview,NULL,0,0,w/2,h/2,SWP_NOZORDER);
	}
	*/
	return TRUE;
}
int populate_sample_list(HWND hlist,int cur_sel)
{
	int i=0;
	if(hlist){
		SendMessage(hlist,CB_RESETCONTENT,0,0);
		while(samples[i]){
			char str[80];
			_snprintf(str,sizeof(str),"sample%i",i+1);
			SendMessage(hlist,CB_ADDSTRING,0,str);
			i++;
		}
		SendMessage(hlist,CB_SETCURSEL,cur_sel,0);
	}
	return i;
}
int check_modify_val(HWND hcheck,int *val)
{
	if(BST_CHECKED==SendMessage(hcheck,BM_GETCHECK,0,0))
		*val=TRUE;
	else
		*val=FALSE;
	return *val;
}
int set_checkbox(HWND hwnd,int idc,int val)
{
	if(val)
		SendDlgItemMessage(hwnd,idc,BM_SETCHECK,BST_CHECKED,0);
	return TRUE;
}
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	extern void compile_program();
	extern char *last_shader_str;
	static int (*callback_set_buffer)(char *);
	switch(msg){
	case WM_INITDIALOG:
		{
			int i;
			HFONT hfont;
			LOGFONT lf={0};
			callback_set_buffer=lparam;
			for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++)
				SendDlgItemMessage(hwnd,IDC_FONTS,CB_ADDSTRING,0,font_names[i].font_name);
			if(IsWindow(heditwin)){
				callback_set_buffer=0;
				hfont=SendMessage(heditwin,WM_GETFONT,0,0);
				if(hfont){
					GetObject(hfont,sizeof(LOGFONT),&lf);
					i=SendDlgItemMessage(hwnd,IDC_FONTS,CB_ADDSTRING,0,lf.lfFaceName);
					SendDlgItemMessage(hwnd,IDC_FONTS,CB_SETCURSEL,i,0);
				}
			}
			populate_sample_list(GetDlgItem(hwnd,IDC_SAMPLELIST),src_sample);
			SendDlgItemMessage(hwnd,IDC_SAMPLELIST,CB_SETCURSEL,src_sample,0);
			set_checkbox(hwnd,IDC_LOAD_PREAMBLE,load_preamble);
			set_checkbox(hwnd,IDC_NEWFORMAT,use_new_format);
			set_checkbox(hwnd,IDC_COMPILE_ON_MODIFY,compile_on_modify);
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_LOAD_PREAMBLE:
			check_modify_val(lparam,&load_preamble);
			compile_program();
			break;
		case IDC_NEWFORMAT:
			check_modify_val(lparam,&use_new_format);
			compile_program();
			break;
		case IDC_COMPILE_ON_MODIFY:
			check_modify_val(lparam,&compile_on_modify);
			break;
		case IDC_OPENINI:
			{
				int explore=FALSE;
				if((GetKeyState(VK_SHIFT)&0x8000) || (GetKeyState(VK_CONTROL)&0x8000))
					explore=TRUE;
				open_ini(hwnd,explore);
			}
			break;
		case IDC_LOADSAMPLE:
			SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDC_SAMPLELIST,CBN_SELCHANGE),GetDlgItem(hwnd,IDC_SAMPLELIST));
			break;
		case IDC_SAMPLELIST:
			{
				HWND hlist=lparam;
				if(HIWORD(wparam)==CBN_SELCHANGE){
					int index=SendMessage(hlist,CB_GETCURSEL,0,0);
					if(index>=0){
						char *str;
						src_sample=index;
						str=samples[src_sample];
						if(callback_set_buffer){
							int tmp=compile_on_modify;
							compile_on_modify=FALSE;
							callback_set_buffer(str);
							compile_program();
							compile_on_modify=tmp;
						}
					}
				}
			}
			break;
		case IDC_TINY_WINDOW:
			toggle_window_size(hwnd);
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
			EndDialog(hwnd,0);
			break;
		case IDOK:
			save_settings();
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

