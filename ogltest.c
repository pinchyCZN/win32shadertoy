#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <richedit.h>
#include "Commctrl.h"
#include "resource.h"
#include "glext.h"

extern const char sample1[],sample2[],sample3[];

LRESULT CALLBACK WndEdit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);


PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;

PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;

PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
PFNGLPROGRAMUNIFORM1FVPROC glProgramUniform1fv;
PFNGLPROGRAMUNIFORM2FPROC glProgramUniform2f;
PFNGLPROGRAMUNIFORM2FVPROC glProgramUniform2fv;
PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;
PFNGLPROGRAMUNIFORM3FVPROC glProgramUniform3fv;
PFNGLPROGRAMUNIFORM4FVPROC glProgramUniform4fv;

PFNGLGETUNIFORMFVPROC glGetUniformfv;
PFNGLGETUNIFORMIVPROC glGetUniformiv;

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
int src_sample=0;
char start_dir[MAX_PATH]={0};

char *preamble=
"uniform vec3      iResolution;           // viewport resolution (in pixels)\r\n"
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
	if(load_preamble){
		int prelen=strlen(preamble);
		if((blen < prelen) || (memcmp(str,preamble,prelen)!=0))
			pre=preamble;
	}
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
	char fname[MAX_PATH]="";
	load_current_path(fname,sizeof(fname));
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
			char *str;
			int rnd;
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

int set_uniform_float(GLuint p,int type,char *name,float *flist,int count)
{
	int result=FALSE;
	GLint loc;
	loc=glGetUniformLocation(p,name);
	if(loc!=-1){
		switch(type){
		case 1:
			glProgramUniform1fv(p,loc,count,flist);
			break;
		case 2:
			glProgramUniform2fv(p,loc,count,flist);
			break;
		case 3:
			glProgramUniform3fv(p,loc,count,flist);
			break;
		case 4:
			glProgramUniform4fv(p,loc,count,flist);
			break;
		}
		if(GL_NO_ERROR!=glGetError())
			printf("error setting %s\n",name);
		else
			result=TRUE;
	}
	return result;
}
int set_uniform_int(GLuint p,char *name,int val)
{
	int result=FALSE;
	GLint loc;
	return 0;
	loc=glGetUniformLocation(p,name);
	if(loc!=-1){
		glProgramUniform1i(p,loc,val);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting %s\n",name);
		else
			result=TRUE;
	}
	return result;
}

int set_vars(GLuint p)
{
	float f[4*3],ftime;
	if(p==0)
		return 0;
	get_time(&ftime);

	f[0]=screenw;
	f[1]=screenh;
	f[2]=0;
	set_uniform_float(p,3,"iResolution",f,1);
	f[0]=ftime;
	set_uniform_float(p,1,"iGlobalTime",f,1);
	set_uniform_float(p,1,"iTime",f,1);

	f[0]=f[1]=f[2]=f[3]=ftime;
	set_uniform_float(p,1,"iChannelTime",f,4);
	{
		int j;
		for(j=0;j<4;j++){
			int x=64,y=64;
			int index;
			get_texture_res(j,&x,&y);
			index=j*3;
			f[index+0]=x;
			f[index+1]=y;
			f[index+2]=0;
		}
		set_uniform_float(p,3,"iChannelResolution",f,4);
		for(j=0;j<4;j++){
			char str[40];
			sprintf(str,"iChannel%i",j);
			set_uniform_int(p,str,j);
		}
	}
	if(lmb_down){
		POINT pt;
		GetCursorPos(&pt);
		MapWindowPoints(NULL,hview,&pt,1);
		clickx=pt.x;
		clicky=pt.y;
		f[0]=pt.x;
		f[1]=screenh-pt.y;

		f[2]=clickx;
		f[3]=screenh-clicky;
		set_uniform_float(p,4,"iMouse",f,1);
	}
	else{
		f[0]=clickx;
		f[1]=screenh-clicky;
		f[2]=0;
		f[3]=0;
		set_uniform_float(p,4,"iMouse",f,1);
	}
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		f[0]=time.wYear;
		f[1]=time.wMonth;
		f[2]=time.wDay;
		f[3]=time.wSecond+time.wMinute*60+time.wHour*60*60;
		set_uniform_float(p,4,"iDate",f,1);
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
		char *tmp;
		int size=len;
		if(size<0 || size>0xA00000)
			size=0xA00000;
		tmp=malloc(size);
		if(tmp && len>0){
			_snprintf(tmp,size,"%s\r\n%s",pre,buf);
			tmp[size-1]=0;
			strncpy(buf,tmp,len);
			buf[len-1]=0;
			free(tmp);
		}
		SendMessage(hedit,EM_GETSEL,&start,&end);
		SendMessage(hedit,EM_SETSEL,0,0);
		SendMessage(hedit,EM_REPLACESEL,FALSE,pre);
		SendMessage(hedit,EM_SETSEL,start,end);
	}
	return TRUE;
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

int save_text(hedit,hstatus)
{
	if(hedit){
		char *s;
		int size=0x10000;
		s=malloc(size);
		if(s){
			FILE *f;
			char path[MAX_PATH]={0};
			GetWindowText(hedit,s,size);
			load_current_path(path,sizeof(path));
			f=fopen(path,"wb");
			if(f){
				int len;
				len=strlen(s);
				fwrite(s,1,len,f);
				fclose(f);
				SetWindowText(hstatus,path);
			}
			free(s);
		}
	}
	return 0;
}
int load_current_path(char *path,int len)
{
	if(path && len>0){
		_snprintf(path,len,"%s\\%s",start_dir,"current.txt");
		path[len-1]=0;
	}
	return 0;
}
int load_current(hedit)
{
	if(hedit){
		char *s;
		int size=0x10000;
		s=malloc(size);
		if(s){
			FILE *f;
			char path[MAX_PATH]={0};
			load_current_path(path,sizeof(path));
			f=fopen(path,"rb");
			if(f){
				CHARRANGE cr;
				cr.cpMax=-1;
				cr.cpMin=-1;
				memset(s,0,size);
				fread(s,1,size-1,f);
				fclose(f);
				SetWindowText(hedit,s);
				SendMessage(hedit,EM_EXSETSEL,0,&cr);
			}
			free(s);
		}
	}
	return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HDC hDC=0;
	static HGLRC hGLRC;
	static int program=0;
	switch(msg){
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
			set_shaders(&program,TRUE);
			load_textures();
			if(heditwin){
				SetWindowPos(heditwin,HWND_TOP,sw/2,0,sw/2,sh/2,SWP_SHOWWINDOW|SWP_NOZORDER);
				restore_window(heditwin,"EDITOR");
				get_ini_value("EDITOR","LOAD_PREAMBLE",&load_preamble);
				PostMessage(hwnd,WM_APP,1,0);
			}
			{
			RECT rect;
			//SetWindowPos(hwnd,HWND_TOP,0,0,sw/2,sh/2,0);
			SetWindowPos(hwnd,HWND_TOP,0,0,sw/(2+4),sh/(2+4),SWP_NOZORDER);
			restore_window(hwnd,"MAIN_WINDOW");

			GetClientRect(hwnd,&rect);
			screenw=rect.right-rect.left;
			screenh=rect.bottom-rect.top;
			set_vars(program);
			}
			SetTimer(hwnd,1000,60,NULL);
			move_console(0,sh/2);
		}
        return 0;
	case WM_APP:
		switch(wparam){
		case 1:
			restore_scroll(GetDlgItem(heditwin,IDC_EDIT1),"EDITOR");
			break;
		}
	case WM_TIMER:
		InvalidateRect(hwnd,NULL,FALSE);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_SETTINGS:
			//DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),heditwin,settings_proc,GetDlgItem(heditwin,IDC_EDIT1));
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
		ReleaseCapture();
		lmb_down=FALSE;
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
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
			float f=0.1f;
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
		if(! ((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000)) ){
			SendMessage(heditwin,WM_CLOSE,0,0);
			save_window_pos(hwnd,"MAIN_WINDOW");
		}
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
	GetCurrentDirectory(sizeof(start_dir),start_dir);
	init_ini_file();
	open_console();
	//create_view(hInstance);
	hview=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SHADER_VIEW),NULL,WndProc);
	ShowWindow(hview,iCmdShow);

	UpdateWindow(hview);

	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR));

	while(GetMessage(&msg,NULL,0,0)){
		//if(msg.message!=WM_TIMER && msg.message!=WM_PAINT)
		//	print_msg(msg.message,msg.lParam,msg.wParam);

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