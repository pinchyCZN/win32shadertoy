#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <richedit.h>
#include "Commctrl.h"
#include "resource.h"
#include "glext.h"

#pragma warning(disable:4113)  //differs in parameter list
#pragma warning(disable:4244) //conversion from blah to float
#pragma warning(disable:4013) //undefined assume return int
#pragma warning(disable:4047) //level indirection
#pragma warning(disable:4024) //diff types for param

extern const char sample1[],sample2[],sample3[];

LRESULT CALLBACK WndEdit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);


PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLDETACHSHADERPROC glDetachShader;
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
DWORD time_start=0;
DWORD time_delta=0;
int frame_counter=0;
HWND hshaderview=0;
HWND heditwin=0;
HINSTANCE ghinstance=0;
int vertid=0,fragid=0,progid=0;
int load_preamble=TRUE;
int use_new_format=TRUE;
int compile_on_modify=FALSE;
int src_sample=0;
char start_dir[MAX_PATH]={0};
char *last_shader_str=0;

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
"uniform vec4      iDate;                 // (year, month, day, time in seconds)\r\n"
"uniform float     iTime;                 // same as global time\r\n"
"uniform float     iTimeDelta;            // render time (in seconds)\r\n"
"uniform float     iFrame;                // shader playback frame\r\n"
;
char *main_preamble=
"void mainImage(out vec4,in vec2);void main(void){mainImage(gl_FragColor,gl_FragCoord);}\r\n"
;

const GLchar *vsh="\
	varying vec4 p;\
	void main(){\
	p=sin(gl_ModelViewMatrix[1]*1.0);\
	gl_Position=gl_Vertex;\
}";

int match_word_state(const char *str,int *index,char a,char prev,int *state,int next)
{
	char b;
	int count=*index;
	if(0==count){
		if(!isspace(a)){
			b=str[count];
			if(isspace(prev)){
				if(a==b)
					*index=*index+1;
				else
					*state=0;
			}else{
				*state=0;
			}
		}
	}else{
		b=str[count];
		*index=count+1;
		if(a!=b)
			*state=0;
		else{
			b=str[count+1];
			if(b==0){
				*state=next;
				*index=0;
			}
		}
	}
	return *state;
}
int needs_main_preamble(char *str)
{
	int i=0,state=0,count=0;
	char a,prev=0;
	int result=FALSE;
	const char *str1="void";
	const char *str2="mainImage";
	for(i=0;a=str[i];i++){
		switch(state){
		case 0:
			count=0;
			if(isspace(a))
				state=1;
			break;
		case 1:
			match_word_state(str1,&count,a,prev,&state,2);
			break;
		case 2:
			match_word_state(str2,&count,a,prev,&state,3);
			break;
		case 3:
			if(isspace(a))
				state=3;
			else if(a=='('){
				state=0;
				result=TRUE;
			}
			break;
		default:
			state=0;
			break;
		}
		prev=a;
		if(result)
			break;
	}
	return result;
}
int setup_shaders()
{
	if(vertid!=0)
		glDeleteShader(vertid);
	if(fragid!=0){
		if(progid!=0)
			glDetachShader(progid,fragid);
		glDeleteShader(fragid);
	}
	if(progid!=0)
		glDeleteProgram(progid);
	vertid=glCreateShader(GL_VERTEX_SHADER);
	fragid=glCreateShader(GL_FRAGMENT_SHADER);
	progid=glCreateProgram();
	glShaderSource(vertid,1,&vsh,NULL);
	glCompileShader(vertid);
	return TRUE;
}
int save_shader_str(const char *str)
{
	int result=FALSE;
	char *tmp;
	int len=strlen(str);
	len++;
	tmp=realloc(last_shader_str,len);
	if(tmp){
		last_shader_str=tmp;
		strncpy(last_shader_str,str,len);
		last_shader_str[len-1]=0;
		result=TRUE;
	}
	return result;
}
int parse_errors(char *str)
{
	char *ptr=str;
	while(1){
		char *line;
		ptr=strstr(ptr,"ERROR:");
		if(!ptr)
			break;
		ptr+=sizeof("ERROR:");
		line=strstr(ptr,":");
		if(line){
			int i,val=0;
			line++;
			for(i=0;i<9;i++){
				char a=line[i];
				if(':'==a)
					break;
				if(a>='0' && a<='9'){
					val*=10;
					val+=a-'0';
				}
			}
			if(val>0){
				set_bookmark(val-1);
			}
		}
	}
	return 0;
}
int gl_check_compile(int id)
{
	int result=0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	set_bookmark(-1);
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
				parse_errors(log);
				free(log);
			}
		}
	}
	return result;
}
int compile_shader_str(const char *str)
{
	int result=FALSE;
	save_shader_str(str);
	glShaderSource(fragid,1,&str,NULL);
	glCompileShader(fragid);
	if(gl_check_compile(fragid)){
		glAttachShader(progid,vertid);
		glAttachShader(progid,fragid);
		glLinkProgram(progid);
		glUseProgram(progid);
		frame_counter=0;
		time_start=GetTickCount();
		set_vars(progid);
		printf("compile success!\n");
		result=TRUE;
	}
	return result;
}

int load_shader_string(const char *str)
{
	char *buf,*pre="";
	int blen,prelen;
	int id=fragid;
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
		save_shader_str(buf);
		glShaderSource(id, 1, &buf, NULL);
		free(buf);
	}
	return TRUE;
}
int get_sample_str(char **str)
{
	int x=rand()%3;
	*str=sample1;
	if(1==x)
		*str=sample2;
	else if(2==x)
		*str=sample3;
	return TRUE;
}
int get_current_fname(char *str,int len)
{
	int result=FALSE;
	get_ini_str("SETTINGS","LAST_FILE",str,len);
	if(str[0]!=0)
		result=TRUE;
	return result;
}
int save_current_fname(char *str)
{
	return write_ini_str("SETTINGS","LAST_FILE",str);
}

void get_time(float *time)
{
	DWORD delta;
	delta=GetTickCount()-time_start;
	if(time)
		*time=(float)delta/(float)1000;
}
void get_delta_time(float *time)
{
	if(time)
		*time=(float)time_delta/(float)1000;
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
	float f[4*3];
	if(p==0)
		return 0;

	f[0]=screenw;
	f[1]=screenh;
	f[2]=0;
	set_uniform_float(p,3,"iResolution",f,1);
	get_time(&f[0]);
	set_uniform_float(p,1,"iGlobalTime",f,1);
	set_uniform_float(p,1,"iTime",f,1);
	f[1]=f[2]=f[3]=f[0];
	set_uniform_float(p,1,"iChannelTime",f,4);

	get_delta_time(&f[0]);
	set_uniform_float(p,1,"iTimeDelta",f,1);
	f[0]=frame_counter;
	set_uniform_float(p,1,"iFrame",f,1);

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
		MapWindowPoints(NULL,hshaderview,&pt,1);
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
	struct GL_FUNC{
		void *(*fptr);
		char *name;
	};
	struct GL_FUNC funcs[]={
		{&glCreateProgram,"glCreateProgram"},
		{&glDeleteProgram,"glDeleteProgram"},
		{&glShaderSource,"glShaderSource"},
		{&glCompileShader,"glCompileShader"},
		{&glCreateShader,"glCreateShader"},
		{&glDeleteShader,"glDeleteShader"},
		{&glAttachShader,"glAttachShader"},
		{&glDetachShader,"glDetachShader"},
		{&glLinkProgram,"glLinkProgram"},
		{&glUseProgram,"glUseProgram"},
		{&glGetShaderiv,"glGetShaderiv"},
		{&glGetShaderInfoLog,"glGetShaderInfoLog"},
		{&glGetUniformLocation,"glGetUniformLocation"},
		{&glProgramUniform1i,"glProgramUniform1i"},
		{&glProgramUniform1f,"glProgramUniform1f"},
		{&glProgramUniform1fv,"glProgramUniform1fv"},
		{&glProgramUniform2f,"glProgramUniform2f"},
		{&glProgramUniform2fv,"glProgramUniform2fv"},
		{&glProgramUniform3f,"glProgramUniform3f"},
		{&glProgramUniform3fv,"glProgramUniform3fv"},
		{&glProgramUniform4fv,"glProgramUniform4fv"},
		{&glGetUniformfv,"glGetUniformfv"},
		{&glGetUniformiv,"glGetUniformiv"},
	};
	int i,result=TRUE;
	for(i=0;i<sizeof(funcs)/sizeof(struct GL_FUNC);i++){
		*funcs[i].fptr=wglGetProcAddress(funcs[i].name);
		if(0==*funcs[i].fptr){
			WCHAR tmp[80];
			_snwprintf(tmp,sizeof(tmp),L"unable to load %S",funcs[i].name);
			MessageBox(NULL,tmp,TEXT("ERROR"),MB_OK|MB_SYSTEMMODAL);
			result=FALSE;
			break;
		}
	}
	return result;
}
int setupPixelFormat(HDC hDC)
{
	int result=FALSE;
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
        MessageBox(WindowFromDC(hDC),TEXT("ChoosePixelFormat failed."),TEXT("Error"),
                MB_ICONERROR | MB_OK);
        return result;
    }

    if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
        MessageBox(WindowFromDC(hDC),TEXT("SetPixelFormat failed."),TEXT("Error"),
                MB_ICONERROR | MB_OK);
        return result;
    }
	result=TRUE;
	return result;
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
		frame_counter=0;
		time_start=GetTickCount();
		set_vars(progid);
		printf("compile success!\n");
		if(hwin)
			SetWindowTextA(hwin,"Shader code");
		result=TRUE;
	}
	else{
		if(hwin)
			SetWindowTextA(hwin,"code ERROR!");
	}
	return result;
}

struct SETTING{
	char *key;
	int *val;
};
const struct SETTING editor_settings[]={
	{"LOAD_PREAMBLE",&load_preamble},
	{"NEWFORMAT",&use_new_format},
	{"COMPILE_ON_MODIFY",&compile_on_modify},
};
int load_settings()
{
	int i;
	for(i=0;i<sizeof(editor_settings)/sizeof(struct SETTING);i++){
		get_ini_value("EDITOR",editor_settings[i].key,editor_settings[i].val);
	}
	return TRUE;
}
int save_settings()
{
	int i;
	for(i=0;i<sizeof(editor_settings)/sizeof(struct SETTING);i++){
		write_ini_value("EDITOR",editor_settings[i].key,*editor_settings[i].val);
	}
	return TRUE;
}
