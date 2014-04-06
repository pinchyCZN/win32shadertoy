#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include "resource.h"
#include "glext.h"
#include "sample.h"

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
static PFNGLPROGRAMUNIFORM1FPROC glProgramUniform1f;
static PFNGLPROGRAMUNIFORM1FVPROC glProgramUniform1fv;
static PFNGLPROGRAMUNIFORM3FPROC glProgramUniform3f;
static PFNGLPROGRAMUNIFORM3FVPROC glProgramUniform3fv;
static PFNGLPROGRAMUNIFORM4FVPROC glProgramUniform4fv;

static PFNGLGETUNIFORMFVPROC glGetUniformfv;
static PFNGLGETUNIFORMIVPROC glGetUniformiv;

int pause=FALSE;
int screenw=1360;
int screenh=768;
HWND hview=0;
HWND heditwin=0;
HINSTANCE ghinstance=0;
int fragid=0,progid=0;

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

int load_frag_shader(GLuint id)
{
	FILE *f;
	char *fname="b:\\test.c";
	f=fopen(fname,"rb");
	if(f){
		int len;
		fseek(f,0,SEEK_END);
		len=ftell(f);
		fseek(f,0,SEEK_SET);
		if(len>0){
			char *buf;
			int blen,prelen;
			blen=len+4;
			prelen=strlen(preamble);
			blen+=prelen;
			buf=malloc(blen);
			if(buf){
				memset(buf,0,blen);
				memcpy(buf,preamble,prelen);
				fread(buf+prelen,1,len,f);
				glShaderSource(id, 1, &buf, NULL);
				if(heditwin){
					SetWindowText(GetDlgItem(heditwin,IDC_EDIT1),buf);
				}
				free(buf);
			}
		}
		fclose(f);
	}
	else{
		printf("cant open file %s\n",fname);
		printf("loading sample instead\n");
		{
			char *buf;
			int blen,prelen;
			blen=strlen(sample1)+4;
			prelen=strlen(preamble);
			blen+=prelen;
			buf=malloc(blen);
			if(buf){
				memset(buf,0,blen);
				memcpy(buf,preamble,prelen);
				strncpy(buf+prelen,sample1,blen-prelen);
				buf[blen-1]=0;
				glShaderSource(id, 1, &buf, NULL);
				if(heditwin){
					SetWindowText(GetDlgItem(heditwin,IDC_EDIT1),buf);
				}
				free(buf);
			}

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
	GLint i;
	float f[4],time;
	get_time(&time);
	if(p==0)
		return 0;
	i=glGetUniformLocation(p,"iResolution");
	if(i!=-1){
		int e;
		f[0]=screenw;
		f[1]=screenh;
		f[2]=0;
		glProgramUniform3fv(p,i,1,&f);
		e=glGetError();
		if(GL_NO_ERROR!=e)
			printf("error setting resolution of %i,%i error=0x%04X\n",screenw,screenh,e);
	}
	i=glGetUniformLocation(p,"iGlobalTime");
	if(i!=-1){
		glProgramUniform1f(p,i,time);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting time\n");
	}
	i=glGetUniformLocation(p,"iChannelTime");
	if(i!=-1){
		float flist[4];
		int j;
		for(j=0;j<4;j++){
			flist[j]=time;
		}
		glProgramUniform1fv(p,i,4,flist);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting channel time\n");
	}
	{
		int j;
		char str[40];
		for(j=0;j<4;j++){
			sprintf(str,"iChannelResolution[%i]",j);
			i=glGetUniformLocation(p,str);
			if(i!=-1){
				float flist[3];
				flist[0]=screenw;
				flist[1]=screenh;
				flist[2]=0;
				glProgramUniform3fv(p,i,1,flist);
				if(GL_NO_ERROR!=glGetError())
					printf("error setting chan rez %i\n",j);
			}
		}
	}
	i=glGetUniformLocation(p,"iMouse");
	if(i!=-1){
		float flist[4];
		int j;
		POINT pt;
		GetCursorPos(&pt);
		for(j=0;j<4;j++){
			if(j==0)
				flist[j]=pt.x;
			else if(j==1)
				flist[j]=pt.y;
			else
				flist[j]=0;
		}
		glProgramUniform4fv(p,i,1,flist);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting mouse\n");
	}
	i=glGetUniformLocation(p,"iDate");
	if(i!=-1){
		float flist[4];
		int j;
		for(j=0;j<4;j++){
			flist[j]=0;
		}
		glProgramUniform4fv(p,i,1,flist);
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
	glProgramUniform1fv=wglGetProcAddress("glProgramUniform1fv");
	glProgramUniform1f=wglGetProcAddress("glProgramUniform1f");

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

LRESULT APIENTRY subclass_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
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
	case WM_KEYDOWN:
		switch(wparam){
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		case VK_F5:
			{
				HWND hparent=GetParent(hwnd);
				SendMessage(GetDlgItem(hparent,IDC_PAUSE),BM_CLICK,0,0);
				SetFocus(hwnd);
			}
			break;
		}
	}
	return CallWindowProc(orig_edit,hwnd,msg,wparam,lparam); 
}

LRESULT CALLBACK WndEdit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		orig_edit=SetWindowLong(GetDlgItem(hwnd,IDC_EDIT1),GWL_WNDPROC,subclass_edit);
		break;
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
		}
		break;
	case WM_SIZE:
		{
			int w,h;
			w=LOWORD(lparam);
			h=HIWORD(lparam);
			SetWindowPos(GetDlgItem(hwnd,IDC_EDIT1),NULL,0,0,w,h-25,SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDOK),NULL,0,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_PAUSE),NULL,100,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
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
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case VK_ESCAPE:
			PostQuitMessage(0);
			EndDialog(hwnd,0);
			break;
		case VK_F5:
			SetActiveWindow(heditwin);
			SendMessage(GetDlgItem(heditwin,IDC_PAUSE),BM_CLICK,0,0);
			SetFocus(hwnd);
			break;
		case VK_F1:
			if(heditwin)
				ShowWindow(heditwin,SW_SHOW);
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
	ghinstance=hInstance;
	open_console();
	//create_view(hInstance);
	hview=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_SHADER_VIEW),NULL,WndProc);
	ShowWindow(hview,iCmdShow);

	UpdateWindow(hview);


	while(GetMessage(&msg,NULL,0,0)){
		if(!IsDialogMessage(heditwin,&msg)){
		//	TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//DispatchMessage(&msg);
	}
	return msg.wParam;

}