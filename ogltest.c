#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <richedit.h>
#include "Commctrl.h"
#include "resource.h"
#include "glext.h"
extern const char *sample1,*sample2,*sample3;

LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);

static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLUSEPROGRAMPROC glUseProgram;

static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;

static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
static PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
static PFNGLPROGRAMUNIFORM1FVPROC glProgramUniform1fv;
static PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
static PFNGLPROGRAMUNIFORM2FVPROC glProgramUniform2fv;
static PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;
static PFNGLPROGRAMUNIFORM3FVPROC glProgramUniform3fv;
static PFNGLPROGRAMUNIFORM4FVPROC glProgramUniform4fv;

static PFNGLGETUNIFORMFVPROC glGetUniformfv;
static PFNGLGETUNIFORMIVPROC glGetUniformiv;

int pause=FALSE;
int screenw=1360;
int screenh=768;
int clickx=0,clicky=0;
int lmb_down=FALSE;
HWND hview=0;
HWND heditwin=0;
HINSTANCE ghinstance=0;
int fragid=0,progid=0;
int load_preamble=TRUE;
int src_sample=1;

char *preamble= "uniform vec3      iResolution;           // viewport resolution (in pixels)\r\n"
"uniform float     iGlobalTime;           // shader playback time (in seconds)\r\n"
"uniform sampler2D iChannel0;          // input channel. XX = 2D/Cube\r\n"
"uniform sampler2D iChannel1;          // input channel. XX = 2D/Cube\r\n"
"uniform sampler2D iChannel2;          // input channel. XX = 2D/Cube\r\n"
"uniform sampler2D iChannel3;          // input channel. XX = 2D/Cube\r\n"
"uniform float     iChannelTime[4];       // channel playback time (in seconds)\r\n"
"uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\r\n"
"uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\r\n"
"uniform vec4      iDate;                 // (year, month, day, time in seconds)\r\n\r\n"
;

int move_console(int x,int y)
{
	BYTE Title[MAX_PATH]; 
	HANDLE hConWnd; 
	GetConsoleTitle(Title,sizeof(Title));
	hConWnd=FindWindow(NULL,Title);
	if(hConWnd)
		SetWindowPos(hConWnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	return 0;
}
void open_console()
{
	char title[MAX_PATH]={0};
	HWND hcon;
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;

	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole();
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),0x4000);

	fflush(stdin);
	hf=_fdopen(hcrt,"w");
	*stdout=*hf;
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW);
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}
const GLchar *vsh="\
	varying vec4 p;\
	void main(){\
	p=sin(gl_ModelViewMatrix[1]*1.0);\
	gl_Position=gl_Vertex;\
}";


int gl_check_compile(int id)
{
	int result=0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if(!result){
		int logLen=0;
		printf("Shader compilation failed!\n");
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
		if (logLen > 0)
		{
			char *log;
			log=malloc(logLen);
			if(log){
				int written=0;
				glGetShaderInfoLog(id, logLen, &written, log);
				printf("Shader log:\n%s", log);
				free(log);
			}
		}
	}
	return result;
}
int load_shader_string(int id,char *str,HWND hedit)
{
	char *buf,*pre="";
	int blen,prelen;
	blen=strlen(str)+4;
	if(load_preamble)
		pre=preamble;
	prelen=strlen(pre);
	blen+=prelen;
	buf=malloc(blen);
	if(buf){
		memset(buf,0,blen);
		memcpy(buf,pre,prelen);
		strncpy(buf+prelen,str,blen-prelen);
		buf[blen-1]=0;
		glShaderSource(id, 1, &buf, NULL);
		if(hedit){
			SetWindowText(hedit,buf);
		}
		free(buf);
	}
	return TRUE;
}
int load_frag_shader(GLuint id)
{
	FILE *f;
	char *fname="b:\\test.c";
	f=fopen(fname,"rb");
	if(f){
		int len;
		printf("loading file %s\n",fname);
		fseek(f,0,SEEK_END);
		len=ftell(f);
		fseek(f,0,SEEK_SET);
		if(len>0){
			char *buf;
			int blen=len+4;
			buf=malloc(blen);
			if(buf){
				memset(buf,0,blen);
				fread(buf,1,len,f);
				load_shader_string(id,buf,GetDlgItem(heditwin,IDC_EDIT1));
				free(buf);
			}

		}
		fclose(f);
	}
	else{
		printf("cant open file %s\n",fname);
		printf("loading sample instead,load other samples from settings dialog\n");
		{
			char *buf,*str;
			int blen,prelen,rnd;
			srand(GetTickCount());
			rnd=rand()&1;
			if(rnd){
				str=sample2;
				src_sample=3;
			}
			else{
				str=sample1;
				src_sample=2;
			}
			str=sample3;
			src_sample=1;
			load_shader_string(id,str,GetDlgItem(heditwin,IDC_EDIT1));
		}
	}
	return 0;
}

int get_time(float *time)
{
	static int init=FALSE;
	static DWORD start;
	DWORD delta;
	if(!init){
		start=GetTickCount();
		init=TRUE;
	}
	delta=GetTickCount()-start;
	if(time)
		*time=(float)delta/(float)1000;
	return delta/1000;
}
int set_vars(GLuint p)
{
	GLint loc;
	float f[4],time;
	get_time(&time);
	if(p==0)
		return 0;
	loc=glGetUniformLocation(p,"iResolution");
	if(loc!=-1){
		int e;
		f[0]=screenw;
		f[1]=screenh;
		f[2]=0;
		glProgramUniform3fv(p,loc,1,&f);
		e=glGetError();
		if(GL_NO_ERROR!=e)
			printf("error setting resolution of %i,%i error=0x%04X\n",screenw,screenh,e);
	}
	loc=glGetUniformLocation(p,"iGlobalTime");
	if(loc!=-1){
		glProgramUniform1f(p,loc,time);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting time\n");
	}
	loc=glGetUniformLocation(p,"iChannelTime");
	if(loc!=-1){
		float flist[4];
		int j;
		for(j=0;j<4;j++){
			flist[j]=time;
		}
		glProgramUniform1fv(p,loc,4,flist);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting channel time\n");
	}
	{
		int j;
		char str[40];
		for(j=0;j<4;j++){
			sprintf(str,"iChannelResolution[%i]",j);
			loc=glGetUniformLocation(p,str);
			if(loc!=-1){
				float flist[3];
				int x=64,y=64;
				get_texture_res(j,&x,&y);
				flist[0]=x;
				flist[1]=y;
				flist[2]=0;
				glProgramUniform3fv(p,loc,1,flist);
				if(GL_NO_ERROR!=glGetError())
					printf("error setting chan rez %i\n",j);
			}
			sprintf(str,"iChannel%i",j);
			loc=glGetUniformLocation(p,str);
			if(loc!=-1){
				int tex=j;
				glProgramUniform1i(p,loc,tex); //texture unit 0-4
				if(GL_NO_ERROR!=glGetError())
					printf("error setting channel texture %i\n",j);
			}
		}
	}
	loc=glGetUniformLocation(p,"iMouse");
	if(loc!=-1){
		float flist[4];
		POINT pt;
		if(lmb_down){
			GetCursorPos(&pt);
			MapWindowPoints(NULL,hview,&pt,1);
			flist[0]=pt.x;
			flist[1]=screenh-pt.y;

			flist[2]=clickx;
			flist[3]=screenh-clicky;
			glProgramUniform4fv(p,loc,1,flist);
			clickx=pt.x;
			clicky=pt.y;
			if(GL_NO_ERROR!=glGetError())
				printf("error setting mouse lmb down\n");
		}
		else{
			flist[0]=clickx;
			flist[1]=screenh-clicky;
			flist[2]=0;
			flist[3]=0;
			glProgramUniform4fv(p,loc,1,flist);
			if(GL_NO_ERROR!=glGetError())
				printf("error setting mouse\n");
		}

	}
	loc=glGetUniformLocation(p,"iDate");
	if(loc!=-1){
		float flist[4];
		SYSTEMTIME time;
		GetLocalTime(&time);
		flist[0]=time.wYear;
		flist[1]=time.wMonth;
		flist[2]=time.wDay;
		flist[3]=time.wSecond+time.wMinute*60+time.wHour*60*60;
		glProgramUniform4fv(p,loc,1,flist);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting date\n");
	}
	return 0;
}
int load_call_table()
{
	glCreateShader=wglGetProcAddress("glCreateShader");
	glCreateProgram=wglGetProcAddress("glCreateProgram");
	glShaderSource=wglGetProcAddress("glShaderSource");
	glCompileShader=wglGetProcAddress("glCompileShader");
	glAttachShader=wglGetProcAddress("glAttachShader");
	glLinkProgram=wglGetProcAddress("glLinkProgram");
	glUseProgram=wglGetProcAddress("glUseProgram");
	glGetShaderInfoLog=wglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv=wglGetProcAddress("glGetShaderiv");

	glGetUniformLocation=wglGetProcAddress("glGetUniformLocation");
	glProgramUniform4fv=wglGetProcAddress("glProgramUniform4fv");
	glProgramUniform3fv=wglGetProcAddress("glProgramUniform3fv");
	glProgramUniform3f=wglGetProcAddress("glProgramUniform3f");
	glProgramUniform2fv=wglGetProcAddress("glProgramUniform2fv");
	glProgramUniform2f=wglGetProcAddress("glProgramUniform2f");
	glProgramUniform1fv=wglGetProcAddress("glProgramUniform1fv");
	glProgramUniform1f=wglGetProcAddress("glProgramUniform1f");
	glProgramUniform1i=wglGetProcAddress("glProgramUniform1i");

	glGetUniformfv=wglGetProcAddress("glGetUniformfv");
	glGetUniformiv=wglGetProcAddress("glGetUniformfv");
	if(glCreateShader==0){
		MessageBox(hview,"Unable to load Open GL extensions","ERROR",MB_OK|MB_SYSTEMMODAL);
		return FALSE;
	}
	return TRUE;
}
int set_shaders(int *program,int fromfile){
	int result=FALSE;
	static GLuint v=0,f=0,p=0;

	if(v==0)
		v = glCreateShader(GL_VERTEX_SHADER);
	if(f==0)
		f = glCreateShader(GL_FRAGMENT_SHADER);
	if(p==0)
		p = glCreateProgram();
	fragid=f;
	progid=p;

	glShaderSource(v, 1, &vsh, NULL);
	glCompileShader(v);
	//glShaderSource(f, 1, &fsh, NULL);
	if(fromfile)
		load_frag_shader(f);

	glCompileShader(f);
	if(gl_check_compile(f)){
		glAttachShader(p,v);
		glAttachShader(p,f);
		glLinkProgram(p);
		set_vars(p);
		glUseProgram(p);
		if(program)
			*program=p;
		result=TRUE;
	}
	return result;
}
int setupPixelFormat(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  /* size */
        1,                              /* version */
        PFD_SUPPORT_OPENGL |
        PFD_DRAW_TO_WINDOW |
        PFD_DOUBLEBUFFER,               /* support double-buffering */
        PFD_TYPE_RGBA,                  /* color type */
        32,                             /* prefered color depth */
        0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
        0,                              /* no alpha buffer */
        0,                              /* alpha bits (ignored) */
        0,                              /* no accumulation buffer */
        0, 0, 0, 0,                     /* accum bits (ignored) */
        16,                             /* depth buffer */
        0,                              /* no stencil buffer */
        0,                              /* no auxiliary buffers */
        PFD_MAIN_PLANE,                 /* main layer */
        0,                              /* reserved */
        0, 0, 0,                        /* no layer, visible, damage masks */
    };
    int pixelFormat;

    pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (pixelFormat == 0) {
        MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
                MB_ICONERROR | MB_OK);
        exit(1);
    }

    if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
        MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
                MB_ICONERROR | MB_OK);
        exit(1);
    }
	/*
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0,sizeof(pfd));
	pfd.cColorBits = pfd.cDepthBits = 32;
	pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	SetPixelFormat(hDC,ChoosePixelFormat(hDC,&pfd),&pfd);
	*/
	return 0;
}
void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
	const GLdouble pi = 3.1415926535897932384626433832795;
	GLdouble fW, fH;
	fH = tan( fovY / 360 * pi ) * zNear;
	fW = fH * aspect;
	glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}
void reshape(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspectiveGL(25.0,(GLfloat)w/(GLfloat)h,.1,10000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,(GLsizei)w,(GLsizei)h);

}
int insert_preamble(HWND hedit,char *buf,int len)
{
	//static const char *_preamble="test";
	char *pre=preamble;
	int prelen=strlen(pre);
	if(memcmp(buf,pre,prelen)!=0){
		int start=0,end=0;
		SendMessage(hedit,EM_GETSEL,&start,&end);
		SendMessage(hedit,EM_SETSEL,0,0);
		SendMessage(hedit,EM_REPLACESEL,FALSE,pre);
		SendMessage(hedit,EM_SETSEL,start,end);
	}
	return TRUE;
}
WNDPROC orig_edit=0;
int edit_busy=FALSE;

int update_status(HWND hedit,HWND hstatus)
{
	int start=0,end=0,line;
	char str[80];
	SendMessage(hedit,EM_GETSEL,&start,&end);
	line=SendMessage(hedit,EM_LINEFROMCHAR,start,0);
	sprintf(str,"%i",line+1);
	SetWindowText(hstatus,str);
	return TRUE;
}
LRESULT CALLBACK find_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hedit=0;
	static int start=0;
	FINDTEXT ftext;
	int pos,dir,fpos;
	static int fontheight=0,timer=0,last_search_pos=0,last_dir=0,at_top=FALSE;
	static char search_text[80]={0};

	switch(msg){
	case WM_INITDIALOG:
		hedit=lparam;
		if(hedit){
			int end=0;
			start=0;
			SendMessage(hedit,EM_GETSEL,&start,&end);
			if(start!=end){
				SendMessage(hedit,WM_COPY,0,0);
				SendMessage(GetDlgItem(hwnd,IDC_SEARCH_BOX),WM_PASTE,0,0);
			}
			else
				SetDlgItemText(hwnd,IDC_SEARCH_BOX,search_text);
			{
				HDC hdc;
				hdc=GetDC(hedit);
				if(hdc!=0){
					TEXTMETRIC tm;
					if(GetTextMetrics(hdc,&tm)!=0)
						fontheight=tm.tmHeight;
					ReleaseDC(hedit,hdc);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if((GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))
				wparam=IDC_SEARCH_UP;
			else
				wparam=IDC_SEARCH_DOWN;
		case IDC_SEARCH_UP:
		case IDC_SEARCH_DOWN:
			if(LOWORD(wparam)==IDC_SEARCH_DOWN)
				dir=FR_DOWN;
			else
				dir=0;
			search_text[0]=0;
			GetWindowText(GetDlgItem(hwnd,IDC_SEARCH_BOX),search_text,sizeof(search_text));
			if(search_text[0]==0)
				break;
			if(last_search_pos==0){
				int end;
				SendMessage(hedit,EM_GETSEL,&pos,&end);
			}
			else{
				pos=last_search_pos;
				if(dir!=last_dir){
					if(dir&FR_DOWN)
						pos++;
					else
						pos--;
				}
				else if(dir&FR_DOWN)
					pos++;

			}
			ftext.lpstrText=search_text;
			ftext.chrg.cpMin=pos;
			if(dir&FR_DOWN)
				ftext.chrg.cpMax=-1;
			else
				ftext.chrg.cpMax=0;
			fpos=SendMessage(hedit,EM_FINDTEXT,dir,&ftext);
			//printf("pos=%i fpos=%i last=%i\n",pos,fpos,last_search_pos);
			if(fpos>=0){
				int line,line2,ltmp;
				RECT rect;
				POINT point;
				GetClientRect(hedit,&rect);
				//printf(" point.x=%i point.y=%i rect.bottom=%i\n",point.x,point.y,rect.bottom);
				line2=SendMessage(hedit,EM_GETFIRSTVISIBLELINE,0,0);
				ltmp=SendMessage(hedit,EM_LINEINDEX,line2,0);
				point.y=0;
				SendMessage(hedit,EM_POSFROMCHAR,&point,ltmp);
				if(point.y<0) //check if first visible line is actually visible
					line2++;
				line=SendMessage(hedit,EM_LINEFROMCHAR,fpos,0);
				point.y=0;
				SendMessage(hedit,EM_POSFROMCHAR,&point,fpos);
				if(line-line2==0){
					if(point.y<0){
						line=0;line2=1;
					}
					else if(point.y>0 && point.y>rect.bottom-fontheight){
						line=1;line2=0;
					}
				}
				else{
					if(point.y>0 && point.y<rect.bottom){
						if(point.y>=rect.bottom-fontheight){
							line=1;line2=0;
						}
						else{
							line=0;line2=0;
						}
					}

				}
				SendMessage(hedit,EM_LINESCROLL,0,line-line2);
				//determine if the text is scrolled up beyond the bottom
				if(line-line2>0){
					line=SendMessage(hedit,EM_GETLINECOUNT,0,0);
					pos=SendMessage(hedit,EM_LINEINDEX,line-1,0);
					point.y=0;
					SendMessage(hedit,EM_POSFROMCHAR,&point,pos);
					if(point.y<rect.bottom-fontheight)
						SendMessage(hedit,EM_SCROLL,SB_LINEDOWN,0);
				}
				point.y=-1;
				SendMessage(hedit,EM_POSFROMCHAR,&point,fpos+strlen(search_text));
				if(point.y>=0 && point.x>=0 &&
					point.y<=rect.bottom && point.x<=rect.right){
					//printf("x=%i y=%i h=%i\n",point.x,point.y,rect.bottom-rect.top);
					ClientToScreen(hedit,&point);
					SetWindowPos(hwnd,NULL,point.x,point.y+fontheight,0,0,SWP_NOZORDER|SWP_NOSIZE);
				}
				//printf("fpos=%i %i %i dif=%i\n",fpos,line,line2,line-line2);
				last_search_pos=fpos;
				if(fpos==0)
					at_top=TRUE;
				else
					at_top=FALSE;
			}
			else{
				POINT point={0,0};
				if(dir&FR_DOWN){
					RECT rect;
					GetWindowRect(hedit,&rect);
					point.x=rect.left;
					point.y=rect.bottom;
				}
				else{
					ClientToScreen(hedit,&point);
				}
			}
			last_dir=dir;
			break;			
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		if(hedit && last_search_pos){
			pos=last_search_pos+strlen(search_text);
			SendMessage(hedit,EM_SETSEL,pos,pos);
		}
		last_search_pos=0;
		break;
	}
	return 0;
}
LRESULT APIENTRY subclass_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_APP:
		{
			int maxlen=0x200000;
			char *buf=malloc(maxlen);
			edit_busy=TRUE;
			if(buf){
				HWND hparent=GetParent(hwnd);
				GetWindowText(hwnd,buf,maxlen);
				buf[maxlen-1]=0;
				if(load_preamble)
					insert_preamble(hwnd,buf,maxlen);
				glShaderSource(fragid, 1, &buf, NULL);
				glCompileShader(fragid);
				if(gl_check_compile(fragid)){
					glAttachShader(progid,fragid);
					glLinkProgram(progid);
					set_vars(progid);
					printf("compile success!\n");
					SetWindowText(hparent,"Shader code");
				}
				else
					SetWindowText(hparent,"code ERROR!");

				free(buf);
			}
			edit_busy=FALSE;
		}
		break;
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_KEYUP:
		update_status(hwnd,GetDlgItem(GetParent(hwnd),IDC_STATUS));
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case 'F':
			if(GetKeyState(VK_CONTROL)&0x8000)
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FIND),hwnd,find_proc,hwnd);	
			break;
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		}
		break;
	}
	return CallWindowProc(orig_edit,hwnd,msg,wparam,lparam); 
}

int compile(HWND hwin)
{
	int result=FALSE;
	glCompileShader(fragid);
	if(gl_check_compile(fragid)){
		glAttachShader(progid,fragid);
		glLinkProgram(progid);
		set_vars(progid);
		printf("compile success!\n");
		SetWindowText(hwin,"Shader code");
		result=TRUE;
	}
	else
		SetWindowText(hwin,"code ERROR!");
	return result;
}



LRESULT CALLBACK WndEdit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		orig_edit=SetWindowLong(GetDlgItem(hwnd,IDC_EDIT1),GWL_WNDPROC,subclass_edit);
		SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETFONT,GetStockObject(ANSI_FIXED_FONT),0);
		SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETEVENTMASK,0,ENM_CHANGE);
		break;
	case WM_HELP:
	case WM_CLOSE:
		ShowWindow(hwnd,SW_HIDE);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_EDIT1:
			switch(HIWORD(wparam)){
			case EN_CHANGE:
				if(!edit_busy)
					PostMessage(GetDlgItem(hwnd,IDC_EDIT1),WM_APP,0,0);
				break;
			}
		case IDC_PAUSE:
			if(IsDlgButtonChecked(hwnd,IDC_PAUSE))
				pause=TRUE;
			else
				pause=FALSE;
			break;
		case IDCANCEL:
			ShowWindow(hwnd,SW_HIDE);
			break;
		case IDC_SETTINGS:
			DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),hwnd,settings_proc,GetDlgItem(hwnd,IDC_EDIT1));
			break;
		}
		break;
	case WM_SIZE:
		{
			int w,h;
			w=LOWORD(lparam);
			h=HIWORD(lparam);
			SetWindowPos(GetDlgItem(hwnd,IDC_EDIT1),NULL,0,0,w,h-25,SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_SETTINGS),NULL,0,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_PAUSE),NULL,100,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			{
				int cx,cy;
				cx=w-200;
				if(cx<0)
					cx=10;
				cy=25;
				SetWindowPos(GetDlgItem(hwnd,IDC_STATUS),NULL,200,h-25,cx,cy,SWP_NOZORDER);
			}
		}
		break;
	}
	return 0;
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HDC hDC=0;
	static HGLRC hGLRC;
	static int program=0;
	switch(msg){
	case WM_TIMER:
		InvalidateRect(hwnd,NULL,FALSE);
		return 0;
    case WM_CREATE:
	case WM_INITDIALOG:
		{
			int sw,sh;
			sw=GetSystemMetrics(SM_CXSCREEN);
			sh=GetSystemMetrics(SM_CYSCREEN);

			hDC=GetDC(hwnd);
			if(hDC)
				setupPixelFormat(hDC);
			hGLRC=wglCreateContext(hDC);
			if(hGLRC){
				wglMakeCurrent(hDC,hGLRC);
				load_call_table();
			}
			heditwin=CreateDialog(ghinstance,MAKEINTRESOURCE(IDD_SHADER_EDIT),hwnd,WndEdit);
			if(heditwin)
				SetWindowPos(heditwin,HWND_TOP,sw/2,0,sw/2,sh/2,SWP_SHOWWINDOW|SWP_NOZORDER);
			set_shaders(&program,TRUE);
			load_textures();
			{
			RECT rect;
			SetWindowPos(hwnd,HWND_TOP,0,0,sw/2,sh/2,0);
			GetClientRect(hwnd,&rect);
			screenw=rect.right-rect.left;
			screenh=rect.bottom-rect.top;
			set_vars(program);
			}
			SetTimer(hwnd,1000,60,NULL);
			move_console(0,sh/2);

		}
        return 0;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_SETTINGS:
			DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),heditwin,settings_proc,GetDlgItem(heditwin,IDC_EDIT1));
			break;
		case IDC_TINY_WINDOW:
			toggle_window_size(0);
			break;
		case IDC_PAUSE:
			{
				HWND hcurrent,hbutton;
				hcurrent=GetFocus();
				hbutton=GetDlgItem(heditwin,IDC_PAUSE);
				SetFocus(hbutton);
				SendDlgItemMessage(heditwin,IDC_PAUSE,BM_CLICK,0,0);
				if(IsWindow(hcurrent))
					SetFocus(hcurrent);
			}
			break;
		}
		break;
	case WM_LBUTTONUP:
		lmb_down=FALSE;
		break;
	case WM_LBUTTONDOWN:
		lmb_down=TRUE;
		clickx=LOWORD(lparam);
		clicky=screenh-HIWORD(lparam);
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case VK_ESCAPE:
			if(lmb_down){
				lmb_down=FALSE;
				break;
			}
			if(IDOK==MessageBox(hwnd,"OK to QUIT?","QUIT?",MB_OKCANCEL)){
				EndDialog(hwnd,0);
				PostQuitMessage(0);
			}
			break;
		case VK_F1:
			if(heditwin){
				int flag=SW_SHOW;
				if(IsWindowVisible(heditwin))
					flag=SW_HIDE;
				ShowWindow(heditwin,flag);
			}
			break;
		}
		break;
	case WM_SIZE:
		{
			screenw=LOWORD(lparam);
			screenh=HIWORD(lparam);
			reshape(screenw,screenh);
			set_vars(program);
		}
		return 0;
		break;
	case WM_PAINT:
		{
			float f=0.1;
			if(pause){
				;
			}
			else{

				glRotatef(f,1,1,1);
				glRecti(-1,-1,1,1);

				if(hDC)
					SwapBuffers(hDC);

				set_vars(program);
			}
//			Sleep(1);
//			InvalidateRect(hwnd,NULL,FALSE);
//			printf(".");
			return 0;
		}
		break;
	case WM_CLOSE:
		exit(0);
		break;
	}
	return 0;
}
int create_view(HINSTANCE hInstance)
{
    WNDCLASS wnd;
	const char *class_name="Win32 SHADERTOY";

	memset(&wnd,0,sizeof(wnd));
	wnd.style=CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	wnd.lpfnWndProc=WndProc;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=hInstance;
	wnd.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wnd.hCursor=LoadCursor(NULL,IDC_ARROW);
	wnd.hbrBackground=GetStockObject(GRAY_BRUSH);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName=class_name;
	RegisterClass(&wnd);
	hview=CreateWindow(class_name,class_name,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE,
		0,0,640,480,NULL,NULL,hInstance,NULL);
	if(!hview){
		MessageBox(NULL,"Could not create main dialog","ERROR",MB_ICONERROR|MB_OK);
		exit(-1);
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int iCmdShow)
{
	MSG msg;
	HACCEL haccel;
	ghinstance=hInstance;
	LoadLibrary("RICHED32.DLL");
	LoadLibrary("RICHED20.DLL");
	open_console();
	//create_view(hInstance);
	hview=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SHADER_VIEW),NULL,WndProc);
	ShowWindow(hview,iCmdShow);

	UpdateWindow(hview);

	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR));

	while(GetMessage(&msg,NULL,0,0)){
		if(haccel!=0)
			TranslateAccelerator(hview,haccel,&msg);
		if(!IsDialogMessage(heditwin,&msg)){
		//	TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//DispatchMessage(&msg);
	}
	return msg.wParam;

}