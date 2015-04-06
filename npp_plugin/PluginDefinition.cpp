//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "DockingDlgInterface.h"
#include "resource.h"
#include <GL/gl.h>
#include "glext.h"
extern "C"{
int get_texture_res(int channel,int *x,int *y);
};
static int lmb_down;
static int clickx,clicky;

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

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
        MessageBox(WindowFromDC(hDC), L"ChoosePixelFormat failed.", L"Error",
                MB_ICONERROR | MB_OK);
    }

    if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
        MessageBox(WindowFromDC(hDC), L"SetPixelFormat failed.", L"Error",
                MB_ICONERROR | MB_OK);
    }
	return 0;
}
int load_call_table(HWND hwnd)
{
	glCreateShader=(PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glCreateProgram=(PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glShaderSource=(PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader=(PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glAttachShader=(PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram=(PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glUseProgram=(PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glGetShaderInfoLog=(PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv=(PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");

	glGetUniformLocation=(PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glProgramUniform4fv=(PFNGLPROGRAMUNIFORM4FVPROC)wglGetProcAddress("glProgramUniform4fv");
	glProgramUniform3fv=(PFNGLPROGRAMUNIFORM3FVPROC)wglGetProcAddress("glProgramUniform3fv");
	glProgramUniform3f=(PFNGLPROGRAMUNIFORM3FPROC)wglGetProcAddress("glProgramUniform3f");
	glProgramUniform2fv=(PFNGLPROGRAMUNIFORM2FVPROC)wglGetProcAddress("glProgramUniform2fv");
	glProgramUniform2f=(PFNGLPROGRAMUNIFORM2FPROC)wglGetProcAddress("glProgramUniform2f");
	glProgramUniform1fv=(PFNGLPROGRAMUNIFORM1FVPROC)wglGetProcAddress("glProgramUniform1fv");
	glProgramUniform1f=(PFNGLPROGRAMUNIFORM1FPROC)wglGetProcAddress("glProgramUniform1f");
	glProgramUniform1i=(PFNGLPROGRAMUNIFORM1IPROC)wglGetProcAddress("glProgramUniform1i");

	glGetUniformfv=(PFNGLGETUNIFORMFVPROC)wglGetProcAddress("glGetUniformfv");
	glGetUniformiv=(PFNGLGETUNIFORMIVPROC)wglGetProcAddress("glGetUniformfv");
	if(glCreateShader==0){
		MessageBox(hwnd,L"Unable to load Open GL extensions",L"ERROR",MB_OK|MB_SYSTEMMODAL);
		return FALSE;
	}
	return TRUE;
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

class SettingsDLG : public DockingDlgInterface
{
public :
	SettingsDLG() : DockingDlgInterface(IDD_ST_SETTINGS){};

	/*
    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
        if (toShow)
            ::SetFocus(::GetDlgItem(_hSelf, IDC_PAUSE));
    };
	*/

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
protected :
	//virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private :
};
class ShaderVIEW : public DockingDlgInterface
{
public :
	HDC hDC;
	HGLRC hGLRC;
	ShaderVIEW() : hDC(0),hGLRC(0),DockingDlgInterface(IDD_SHADER_VIEW){};
	int create_gl_context(){
		int result=FALSE;
		if(hGLRC)
			return TRUE;
		if(_hSelf){
			hDC=GetDC(_hSelf);
			if(hDC)
				setupPixelFormat(hDC);
			hGLRC=wglCreateContext(hDC);
			if(hGLRC){
				wglMakeCurrent(hDC,hGLRC);
				load_call_table(_hSelf);
				result=TRUE;
			}
		}
		return result;
	}
	/*
    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
   };
   */
	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
protected :
	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam){
		switch (message) 
		{
			case WM_NOTIFY: 
			{
				LPNMHDR	pnmh	= (LPNMHDR)lParam;

				if (pnmh->hwndFrom == _hParent)
				{
					switch (LOWORD(pnmh->code))
					{
						case DMN_CLOSE:
						{
							break;
						}
						case DMN_FLOAT:
						{
							_isFloating = true;
							break;
						}
						case DMN_DOCK:
						{
							_isFloating = false;
							break;
						}
						default:
							break;
					}
				}
				break;
			}
			default:
				break;
		}
		return FALSE;
	}
private :
};

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;
HINSTANCE hinstance;
ShaderVIEW shader_view;
SettingsDLG settings;
int menu_start;
int menu_settings;

static int printf(char *fmt,...)
{
	return 0;
}
static int sprintf(char *buf,char *fmt,...)
{
	return 0;
}
int set_vars(GLuint p)
{
	GLint loc;
	float f[4],time;
	get_time(&time);
	if(p==0)
		return 0;
	loc=glGetUniformLocation(p,(GLchar *)"iResolution");
	if(loc!=-1 && shader_view.getHSelf()){
		int e;
		int screenw,screenh;
		screenw=shader_view.getWidth();
		screenh=shader_view.getHeight();
		f[0]=screenw;
		f[1]=screenh;
		f[2]=0;
		glProgramUniform3fv(p,loc,1,f);
		e=glGetError();
		if(GL_NO_ERROR!=e)
			printf("error setting resolution of %i,%i error=0x%04X\n",screenw,screenh,e);
	}
	loc=glGetUniformLocation(p,(GLchar *)"iGlobalTime");
	if(loc!=-1){
		glProgramUniform1f(p,loc,time);
		if(GL_NO_ERROR!=glGetError())
			printf("error setting time\n");
	}
	loc=glGetUniformLocation(p,(GLchar *)"iChannelTime");
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
			loc=glGetUniformLocation(p,(GLchar *)str);
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
			loc=glGetUniformLocation(p,(GLchar *)str);
			if(loc!=-1){
				int tex=j;
				glProgramUniform1i(p,loc,tex); //texture unit 0-4
				if(GL_NO_ERROR!=glGetError())
					printf("error setting channel texture %i\n",j);
			}
		}
	}
	loc=glGetUniformLocation(p,(GLchar *)"iMouse");
	if(loc!=-1){
		float flist[4];
		POINT pt;
		int screenh;
		screenh=shader_view.getHeight();
		if(lmb_down){
			GetCursorPos(&pt);
			MapWindowPoints(NULL,shader_view.getHSelf(),&pt,1);
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
	loc=glGetUniformLocation(p,(GLchar *)"iDate");
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


//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	hinstance=(HINSTANCE)hModule;
	settings.init(hinstance,NULL);
	shader_view.init(hinstance,NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}



//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}
void dialogsInit()
{
	settings.init(hinstance,nppData._nppHandle);
	shader_view.init(hinstance,nppData._nppHandle);
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void show_dialogs(int show_settings)
{
	tTbData	data = {0};
	if(!shader_view.isCreated()){
		shader_view.create(&data);
		data.uMask = DWS_DF_FLOATING;
		data.pszModuleName = shader_view.getPluginFileName();
		// the dlgDlg should be the index of funcItem where the current function pointer is
		data.dlgID = menu_start;
		data.rcFloat.right=200;
		data.rcFloat.bottom=200;
		::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		shader_view.create_gl_context();
	}
	if(!settings.isCreated()){
		settings.create(&data);
		// define the default docking behaviour
		data.uMask = DWS_DF_FLOATING; //DWS_DF_CONT_RIGHT;
		data.pszModuleName = settings.getPluginFileName();

		// the dlgDlg should be the index of funcItem where the current function pointer is
		data.dlgID = menu_settings;
		::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		settings.display(true);
		memset(&data.rcFloat,0,sizeof(data.rcFloat));
	}
	else if(show_settings){
		int t=!settings.isVisible();
		settings.display(t);
	}
	//shader_view.
	shader_view.display(true);
	shader_view.updateDockingDlg();
	//HWND hwnd=shader_view.updateDockingDlg();
	//SendMessage(hwnd,WM_LBUTTONDBLCLK,0,0);

	//shader_view.toggleActiveTb();
	//shader_view.
}

void start_shadertoy()
{
	show_dialogs(true);
}
void show_settings()
{
	if(shader_view.isCreated())
		show_dialogs(true);
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    menu_start=setCommand(0, TEXT("Start shadertoy"), start_shadertoy, NULL, false);
    menu_settings=setCommand(1, TEXT("Show settings"), show_settings, NULL, false);
	dialogsInit();
}