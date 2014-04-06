// Chris Thornborrow (auld/sek/taj/copabars)
// If you use this code please credit...blahblah
// Example OGL + shaders in 1k
// Requires crinkler - magnificent tool
// VS2005 modifications by benny!weltenkonstrukteur.de from dbf
//    Greets!
// NOTE: DX will beat this no problem at all due to OpenGL forced
// to import shader calls as variables..nontheless we dont need
// d3dxblahblah to be loaded on users machine.
#include <windows.h>
#include <stdio.h>
#include <GL/gl.h>
#include "glext.h"
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


char *preamble= "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
"uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
"uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
"uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
"uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
"uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
;


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
// NOTE: in glsl it is legal to have a fragment shader without a vertex shader
//  Infact ATi/AMD  drivers allow this but unwisely forget to set up variables for
// the fragment shader - thus all GLSL programs must have a vertex shader :-(
// Thanks ATI/AMD
// This is pretty dirty...note we do not transform the rectangle but we do use
// glRotatef to pass in a value we can use to animate...avoids one more getProcAddress later
const GLchar *vsh="\
varying vec4 p;\
void main(){\
p=sin(gl_ModelViewMatrix[1]*1.0);\
gl_Position=gl_Vertex;\
}";
// an iterative function for colour
const GLchar *fsh="\
varying vec4 p;\
void main(){\
float r,t,j;\
vec4 v=gl_FragCoord/40.0-1.0;\
r=v.x*p.r;\
for(int j=0;j<7;j++){\
t=v.x+p.r*p.g;\
v.x=t*t-v.y*v.y+r;\
v.y=p.g*3.0*t*v.y+v.y;\
}\
/*gl_FragColor=vec4(mix(p,vec4(t),max(t,v.x)));*/\
gl_FragColor=gl_FragCoord/100;\
}";
//p.g*3.0*t*v.y+i;

int gl_check_compile(int id)
{
	int result=0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (!result){
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
				int result;
				memset(buf,0,blen);
				memcpy(buf,preamble,prelen);
				fread(buf+prelen,1,len,f);
				glShaderSource(id, 1, &buf, NULL);
				free(buf);
			}
		}
		fclose(f);
	}
	else
		printf("cant open file %s\n",fname);
	return 0;
}

int get_time()
{
	static int init=FALSE;
	static DWORD start;
	DWORD delta;
	if(!init){
		start=GetTickCount();
		init=TRUE;
	}
	delta=GetTickCount()-start;
	return delta/1000;
}
int set_vars(GLuint p)
{
	GLint i;
	float f[4],time;
	time=get_time();
	i=glGetUniformLocation(p,"iResolution");
	if(i!=0){
		f[0]=1360;
		f[1]=768;
		f[2]=0;
		glProgramUniform3fv(p,i,1,&f);
		if(GL_NO_ERROR!=glGetError())
			printf("error\n");
	}
	i=glGetUniformLocation(p,"iGlobalTime");
	if(i!=0){
		glProgramUniform1f(p,i,time);
	}
	i=glGetUniformLocation(p,"iChannelTime");
	if(i!=0){
		float flist[4];
		int j;
		for(j=0;j<4;j++){
			flist[j]=time;
		}
		glProgramUniform4fv(p,i,1,flist);
	}

}
int setShaders(){
	GLuint v,f,p;
	
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
	
	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);	
	p = glCreateProgram();
	glShaderSource(v, 1, &vsh, NULL);
	glCompileShader(v);
	//glShaderSource(f, 1, &fsh, NULL);
	load_frag_shader(f);
	glCompileShader(f);
	gl_check_compile(f);

	glAttachShader(p,v);
	glAttachShader(p,f);
	glLinkProgram(p);
	set_vars(p);
	glUseProgram(p);
	
	return p;
}
// force them to set everything to zero by making them static
static PIXELFORMATDESCRIPTOR pfd;
static DEVMODE dmScreenSettings;
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow )
{
	HDC hDC;
	int program;
	open_console();
  dmScreenSettings.dmSize=sizeof(dmScreenSettings);		
  dmScreenSettings.dmPelsWidth	= 1360;
  dmScreenSettings.dmPelsHeight= 768;
// 	  dmScreenSettings.dmBitsPerPel	= 32;
// its risky to remove the flag and bits but probably safe on compo machine :-)
  dmScreenSettings.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;
 // ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);  
  // minimal windows setup code for opengl  
  pfd.cColorBits = pfd.cDepthBits = 32;
  pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;	
  // "HDC hDC" changed 19. April 2007 by benny!weltenkonstrukteur.de
  hDC = GetDC( CreateWindow("edit", 0, WS_POPUP|WS_VISIBLE|WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0) );
  SetPixelFormat ( hDC, ChoosePixelFormat ( hDC, &pfd) , &pfd );
  wglMakeCurrent ( hDC, wglCreateContext(hDC) );
  program=setShaders();
  ShowCursor(FALSE); 
   //**********************
   // NOW THE MAIN LOOP...
   //**********************
   // there is no depth test or clear screen...as we draw in order and cover
   // the whole area of the screen.
   do {
        //dodgy, no oglLoadIdentity- might break...
        // change the first number to alter speed of intro...smaller is slower
        // this is the fast version
	   {
		static float f=.1;
        glRotatef(f,1,1,1);
	   }
        // draw a single flat rectangle on screen...
        glRecti(-1,-1,1,1);
        SwapBuffers(hDC);
		Sleep(1);
		set_vars(program);
   } while ( !GetAsyncKeyState(VK_ESCAPE) );   
}