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

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <GL/gl.h>
#include "PluginInterface.h"
#include "menuCmdID.h"
#include "resource.h"

static NppData nppData={0};
const int nbFunc = 5;
static FuncItem funcItem[nbFunc]={0};
static ShortcutKey skeys[nbFunc]={0};
static HWND hshaderview;


HWND ghconsole=0;
int move_console(int x,int y,int w,int h)
{
	if(ghconsole!=0)
		SetWindowPos(ghconsole,0,x,y,w,h,SWP_NOZORDER);
	return 0;
}
#define _O_TEXT         0x4000  /* file mode is text (translated) */
extern "C" int _open_osfhandle(long,int);
void open_console()
{
	HWND hcon;
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;
	static HWND (*GetConsoleWindow)(void)=0;

	if(consolecreated==TRUE)
	{
		if(ghconsole!=0)
			ShowWindow(ghconsole,SW_SHOW);
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole();
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w");
	*stdout=*hf;
	setvbuf(stdout,NULL,_IONBF,0);
	consolecreated=TRUE;
	if(GetConsoleWindow==0){
		HMODULE hmod=LoadLibrary(TEXT("kernel32.dll"));
		if(hmod!=0){
			GetConsoleWindow=(HWND (*)(void))GetProcAddress(hmod,"GetConsoleWindow");
			if(GetConsoleWindow!=0){
				ghconsole=GetConsoleWindow();
			}
		}
	}

}
void hide_console()
{
	if(ghconsole!=0){
		ShowWindow(ghconsole,SW_HIDE);
		SetForegroundWindow(ghconsole);
	}
}
int assert(bool cond,char *msg)
{
#ifdef _DEBUG
	if(!cond)
		MessageBoxA(NULL,msg,"ASSERT",MB_OK|MB_SYSTEMMODAL);
#endif
	return cond;
}

extern "C" {
int setupPixelFormat(HDC hDC);
int load_call_table();
int set_shaders(int *program,int fromfile);
int load_textures();
int restore_window(HWND hwnd,const char *WINDOW_NAME);
int set_vars(GLuint p);
int toggle_window_size(HWND hwnd);
void reshape(int w, int h);
int save_window_pos(HWND hwnd,const char *WINDOW_NAME);
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
int compile_shader_str(char *str);
int load_settings();
extern HINSTANCE ghinstance;
extern int screenw,screenh;
extern int lmb_down,clickx,clicky;
extern int fragid,progid;
extern int pause,load_preamble,use_new_format;
extern char ini_file[MAX_PATH];
extern char start_dir[MAX_PATH];
extern char *preamble,*postamble;
}


LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HDC hDC=0;
	static HGLRC hGLRC;
	static int program=0;
	static int timer_id=0;
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
			set_shaders(&program,TRUE);
			load_textures();
			{
			RECT rect;
			//SetWindowPos(hwnd,HWND_TOP,0,0,sw/2,sh/2,0);
			SetWindowPos(hwnd,HWND_TOP,0,0,sw/(2+4),sh/(2+4),0);
			restore_window(hwnd,"MAIN_WINDOW");

			GetClientRect(hwnd,&rect);
			screenw=rect.right-rect.left;
			screenh=rect.bottom-rect.top;
			set_vars(program);
			}
			timer_id=SetTimer(hwnd,1000,60,NULL);
			hshaderview=hwnd;
			pause=FALSE;
			//move_console(0,sh/2);
		}
        return 0;
	case WM_APP:
		break;
	case WM_TIMER:
		InvalidateRect(hwnd,NULL,FALSE);
		return 0;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_TINY_WINDOW:
			toggle_window_size(0);
			break;
		case IDC_PAUSE:
			{
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
			if(IDOK==MessageBox(hwnd,TEXT("OK to QUIT?"),TEXT("QUIT?"),MB_OKCANCEL)){
				EndDialog(hwnd,0);
				PostQuitMessage(0);
			}
			break;
		case VK_F1:
			break;
		}
		break;
	case WM_SIZE:
		{
			screenw=LOWORD(lparam);
			screenh=HIWORD(lparam);
			printf("screen res x=%i,y=%i\n",screenw,screenh);
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
			return 0;
		}
		break;
	case WM_CLOSE:
		if(! ((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000)) ){
			save_window_pos(hwnd,"MAIN_WINDOW");
		}
		hide_console();
		program=0;
		KillTimer(hwnd,timer_id);
		hshaderview=0;
		wglDeleteContext(hGLRC);
		EndDialog(hwnd,0);
		break;
	}
	return 0;
}

int file_exist(WCHAR *path)
{
	int attrib;
	attrib=GetFileAttributes(path);
	if((attrib!=0xFFFFFFFF) && (!(attrib&FILE_ATTRIBUTE_DIRECTORY)))
		return TRUE;
	else
		return FALSE;
}
void start_shadertoy()
{
#ifdef _DEBUG
		open_console();
#endif
	if(hshaderview==0){
		CreateDialog((HINSTANCE)ghinstance,MAKEINTRESOURCE(IDD_SHADER_VIEW),NULL,(DLGPROC)WndProc);
		SetWindowPos(hshaderview,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
		WCHAR str[MAX_PATH]={0};
		SendMessage(nppData._nppHandle,NPPM_GETPLUGINSCONFIGDIR,sizeof(str),(LPARAM)str);
		if(str[0]!=0){
			_snwprintf(str,sizeof(str)/sizeof(TCHAR),L"%s\\%s",str,L"CURRENT.txt");
			if(file_exist(str)){
				SendMessage(nppData._nppHandle,NPPM_DOOPEN,0,(LPARAM)str);
				SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE,0,(LPARAM)str);
			}else{
				PostMessage(nppData._nppHandle,NPPM_MENUCOMMAND,0,IDM_FILE_NEW);
			}
			PostMessage(nppData._nppHandle,NPPM_SETCURRENTLANGTYPE,0,L_C);
			
		}
	}else{
		ShowWindow(hshaderview,SW_SHOWNORMAL);
	}
}
void stop_shadertoy()
{
	if(hshaderview!=0){
		PostMessage(hshaderview,WM_CLOSE,0,0);
	}
}
void show_settings()
{
	DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),nppData._nppHandle,(DLGPROC)settings_proc,NULL);
}
int insert_selection(HWND hscint,char *str)
{
	int x,y,line;
	x=SendMessage(hscint,SCI_GETSELECTIONSTART,0,0);
	y=SendMessage(hscint,SCI_GETSELECTIONEND,0,0);
	line=SendMessage(hscint,SCI_GETFIRSTVISIBLELINE,0,0);
	SendMessage(hscint,SCI_SETSELECTIONSTART,0,0);
	SendMessage(hscint,SCI_SETSELECTIONEND,0,0);
	SendMessage(hscint,SCI_REPLACESEL,0,(LPARAM)str);
	SendMessage(hscint,SCI_SETSELECTIONSTART,x,0);
	SendMessage(hscint,SCI_SETSELECTIONEND,y,0);
	return TRUE;
}
void compile_program()
{
#define MAX_BUF 0x400000
	if(0==hshaderview)
		return;
	int len;
	HWND hscint=nppData._scintillaMainHandle;
	len=SendMessage(hscint,SCI_GETLENGTH,0,0);
	len++;
	if(len>MAX_BUF)
		len=MAX_BUF;

	char *buf=(char*)malloc(len);
	if(buf){
		SendMessage(hscint,SCI_GETTEXT,len,(LPARAM)buf);
		buf[len-1]=0;
		if(load_preamble){
			char *f=strstr(buf,preamble);
			if(0==f){
				char *tmp=(char*)malloc(MAX_BUF);
				if(tmp){
					insert_selection(hscint,preamble);
					_snprintf(tmp,MAX_BUF,"%s\r\n%s",preamble,buf);
					tmp[MAX_BUF-1]=0;
					free(buf);
					buf=tmp;
					len=MAX_BUF;
				}
			}
		}
		if(use_new_format){
			char *f=strstr(buf,postamble);
			if(0==f){
				char *tmp=(char*)malloc(MAX_BUF);
				if(tmp){
					insert_selection(hscint,postamble);
					_snprintf(tmp,MAX_BUF,"%s\r\n%s",buf,postamble);
					tmp[MAX_BUF-1]=0;
					free(buf);
					buf=tmp;
					len=MAX_BUF;
				}
			}
		}
		buf[len-1]=0;
		printf("%s\n",buf);
		compile_shader_str(buf);
		if(buf)
			free(buf);
	}
}
void pause_program()
{
	const char *title="Shader Toy";
	pause=!pause;
	if(pause)
		title="Paused";
	if(hshaderview)
		SetWindowTextA(hshaderview,title);
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
			ghinstance=(HINSTANCE)hModule;
		break;
		case DLL_PROCESS_DETACH:
		break;
		case DLL_THREAD_ATTACH:
		break;
		case DLL_THREAD_DETACH:
		break;
    }

    return TRUE;
}

static bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc,
					   int KEY,bool isCtrl,bool isAlt,bool isShift,
					   bool check0nInit)
{
	ShortcutKey *key=0;
	assert(index<nbFunc,"setCommand index out of bounds");
    if(index>=nbFunc)
        return false;
	if(!pFunc)
        return false;
	if(0!=KEY){
		skeys[index]._key=KEY;
		skeys[index]._isCtrl=isCtrl;
		skeys[index]._isAlt=isAlt;
		skeys[index]._isShift=isShift;
		key=&skeys[index];
	}

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = key;
    return true;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	int index=0;
    setCommand(index++, TEXT("Start shadertoy"),start_shadertoy,0,0,0,0,false);
    setCommand(index++, TEXT("Stop shadertoy"),stop_shadertoy,0,0,0,0, false);
    setCommand(index++, TEXT("Pause"),pause_program,'P',true,0,0,false);
    setCommand(index++, TEXT("Show settings"), show_settings,VK_F1,true,0,0,false);
    setCommand(index++, TEXT("Compile"),compile_program,VK_RETURN,true,0,0,false);

	WCHAR str[MAX_PATH]={0};
	SendMessage(nppData._nppHandle,NPPM_GETPLUGINSCONFIGDIR,sizeof(str),(LPARAM)str);
	char tmp[MAX_PATH]={0};
	wcstombs(tmp,str,sizeof(tmp));
	strncpy(ini_file,tmp,sizeof(ini_file));
	if(ini_file[0]!=0){
		_snprintf(ini_file,sizeof(ini_file),"%s\\%s",ini_file,"SHADER_TOY.INI");
		strncpy(start_dir,tmp,sizeof(start_dir));
	}
	load_settings();
#ifdef _DEBUG
	start_shadertoy();
#endif
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return TEXT("Shadertoy plugin");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case NPPN_SHUTDOWN:
		{
//			commandMenuCleanUp();
		}
		break;

		default:
			return;
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
