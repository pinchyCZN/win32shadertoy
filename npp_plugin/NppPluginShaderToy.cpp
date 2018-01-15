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

int assert(bool cond,char *msg)
{
#ifdef _DEBUG
	if(!cond)
		MessageBoxA(NULL,msg,"ASSERT",MB_OK|MB_SYSTEMMODAL);
#endif
	return cond;
}

extern "C" {
void open_console();
void hide_console();
void move_console(int,int,int,int);
DWORD set_console_icon(HICON hicon);
int setupPixelFormat(HDC hDC);
int load_call_table();
int setup_shaders();
int load_textures();
int restore_window(HWND hwnd,const char *WINDOW_NAME);
int set_vars(GLuint p);
int toggle_window_size(HWND hwnd);
void reshape(int w, int h);
int save_window_pos(HWND hwnd,const char *WINDOW_NAME);
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
int compile_shader_str(char *str);
int load_settings();
int needs_main_preamble(char *str);
int get_sample_str(char **str);
int get_current_fname(char *str,int len);
int save_current_fname(char *str);
extern HINSTANCE ghinstance;
extern int screenw,screenh;
extern int lmb_down,clickx,clicky;
extern DWORD time_delta;
extern int frame_counter;
extern int fragid,progid;
extern int pause,load_preamble,use_new_format,compile_on_modify;
extern char ini_file[MAX_PATH];
extern char start_dir[MAX_PATH];
extern char *preamble,*main_preamble;
extern char *last_shader_str;
extern HWND hshaderview;
extern HWND ghconsole;
}

int snap_console(HWND hwin)
{
	RECT rect={0};
	GetWindowRect(hwin,&rect);
	move_console(rect.left,rect.bottom,0,0);
	return TRUE;
}
int print_info()
{
	printf("time delta=%i\n",time_delta);
	printf("pause=%i\n",pause);
	printf("click x/y=%i %i\n",clickx,clicky);
	return 0;
}
extern "C" int set_bookmark(int line)
{
	HWND hscint;
	hscint=nppData._scintillaMainHandle;
	if(!hscint)
		return 0;
	if(line<0)
		return SendMessage(hscint,SCI_MARKERDELETEALL,-1,0);
	else
		return SendMessage(hscint,SCI_MARKERADD,line,24);
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HDC hDC=0;
	static HGLRC hGLRC;
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
			setup_shaders();
			load_textures();
			{
			RECT rect;
			SetWindowPos(hwnd,HWND_TOP,0,0,sw/(2+4),sh/(2+4),0);
			restore_window(hwnd,"MAIN_WINDOW");
			SetClassLong(hwnd,GCL_HICON,(LONG)LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON1)));

			GetClientRect(hwnd,&rect);
			screenw=rect.right-rect.left;
			screenh=rect.bottom-rect.top;
			set_vars(progid);
			}
			timer_id=SetTimer(hwnd,1000,60,NULL);
			hshaderview=hwnd;
			pause=FALSE;
		}
        return 0;
	case WM_LBUTTONDBLCLK:
		if(IsZoomed(hwnd))
			ShowWindow(hwnd,SW_SHOWNORMAL);
		else
			ShowWindow(hwnd,SW_SHOWMAXIMIZED);
		break;
	case WM_APP:
		break;
	case WM_TIMER:
		InvalidateRect(hwnd,NULL,FALSE);
		return 0;
	case WM_ACTIVATEAPP:
		break;
	case WM_COMMAND:
		/*
		switch(LOWORD(wparam)){
		default:
			break;
		}
		*/
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
	case WM_RBUTTONDOWN:
		print_info();
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case VK_ESCAPE:
			break;
		case VK_F1:
			print_info();
			return TRUE;
			break;
		}
		break;
	case WM_SIZE:
		{
			screenw=LOWORD(lparam);
			screenh=HIWORD(lparam);
			printf("screen res x=%i,y=%i\n",screenw,screenh);
			reshape(screenw,screenh);
			set_vars(progid);
			snap_console(hwnd);
		}
		return 0;
		break;
	case WM_MOVE:
		snap_console(hwnd);
		break;
	case WM_PAINT:
		{
			float f=0.1f;
			if(pause){
				;
			}
			else{
				static DWORD tick=0;
				DWORD ctick;
				glRotatef(f,1,1,1);
				glRecti(-1,-1,1,1);

				if(hDC)
					SwapBuffers(hDC);

				set_vars(progid);
				frame_counter++;
				ctick=GetTickCount();
				time_delta=ctick-tick;
				tick=ctick;
			}
			return 0;
		}
		break;
	case WM_CLOSE:
		if(! ((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000)) ){
			save_window_pos(hwnd,"MAIN_WINDOW");
		}
		{
			WCHAR tmp[MAX_PATH]={0};
			char str[MAX_PATH]={0};
			SendMessage(nppData._nppHandle,NPPM_GETFULLCURRENTPATH,sizeof(tmp)/sizeof(WCHAR),(LPARAM)tmp);
			if(tmp[0]!=0){
				wcstombs(str,tmp,sizeof(str));
				save_current_fname(str);
			}
		}
		hide_console();
		KillTimer(hwnd,timer_id);
		hshaderview=0;
		wglDeleteContext(hGLRC);
		EndDialog(hwnd,0);
		break;
	}
	return 0;
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

extern "C" void compile_program()
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
			if(needs_main_preamble(buf)){
				char *f=strstr(buf,main_preamble);
				if(0==f){
					char *tmp=(char*)malloc(MAX_BUF);
					if(tmp){
						printf("new format detected\n");
						insert_selection(hscint,main_preamble);
						_snprintf(tmp,MAX_BUF,"%s\r\n%s",buf,main_preamble);
						tmp[MAX_BUF-1]=0;
						free(buf);
						buf=tmp;
						len=MAX_BUF;
					}
				}
			}
		}
		buf[len-1]=0;
		//printf("%s\n",buf);
		compile_shader_str(buf);
		if(buf)
			free(buf);
	}
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
int set_scintilla_buffer(char *str)
{
	int result=FALSE;
	if(str && nppData._scintillaMainHandle){
		SendMessage(nppData._scintillaMainHandle,SCI_SETTEXT,0,(LPARAM)str);
		result=TRUE;
	}
	return result;
}
void start_shadertoy()
{
	open_console();
	set_console_icon((HICON)LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON2)));
	if(hshaderview==0){
		CreateDialog((HINSTANCE)ghinstance,MAKEINTRESOURCE(IDD_SHADER_VIEW),nppData._nppHandle,(DLGPROC)WndProc);
		SetWindowPos(hshaderview,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER);
		WCHAR str[MAX_PATH]={0};
		char tmp[MAX_PATH]={0};
		if(get_current_fname(tmp,sizeof(tmp))){
			mbstowcs(str,tmp,sizeof(str)/sizeof(WCHAR));
			str[sizeof(str)/sizeof(WCHAR)-1]=0;
		}
		if(str[0]!=0 && file_exist(str)){
			SendMessage(nppData._nppHandle,NPPM_DOOPEN,0,(LPARAM)str);
			SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE,0,(LPARAM)str);
			SendMessage(nppData._nppHandle,NPPM_SETCURRENTLANGTYPE,0,L_C);
		}else{
			if(!SendMessage(nppData._nppHandle,NPPM_SWITCHTOFILE,0,(LPARAM)str)){
				int buf_id;
				char *tmp=0;
				SendMessage(nppData._nppHandle,NPPM_MENUCOMMAND,0,IDM_FILE_NEW);
				buf_id=SendMessage(nppData._nppHandle,NPPM_GETCURRENTBUFFERID,0,0);
				if(str[0]!=0)
					SendMessage(nppData._nppHandle,NPPM_INTERNAL_SETFILENAME,buf_id,(LPARAM)str);
				SendMessage(nppData._nppHandle,NPPM_SETCURRENTLANGTYPE,0,L_C);
				get_sample_str(&tmp);
				if(tmp!=0)
					set_scintilla_buffer(tmp);
			}
		}
		compile_program();
		SendMessage(nppData._nppHandle,IDM_SEARCH_CLEAR_BOOKMARKS,0,0);
	}else{
		ShowWindow(hshaderview,SW_SHOWNORMAL);
	}
	if(hshaderview){
		RECT rect;
		GetWindowRect(hshaderview,&rect);
		move_console(rect.left,rect.bottom,0,0);
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
	DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),nppData._nppHandle,(DLGPROC)settings_proc,(LPARAM)set_scintilla_buffer);
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
		DWORD attr;
		strncpy(start_dir,tmp,sizeof(start_dir));
		attr=GetFileAttributesA(start_dir);
		if(attr==MAXDWORD){
			CreateDirectoryA(start_dir,NULL);
		}
		_snprintf(ini_file,sizeof(ini_file),"%s\\%s",ini_file,"SHADER_TOY.INI");
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
//	if(notifyCode->nmhdr.code!=2013)
//		printf("code=%i\n",notifyCode->nmhdr.code);
	switch (notifyCode->nmhdr.code) 
	{
	case SCN_MODIFIED:
		if(notifyCode->modificationType&(SC_MOD_INSERTTEXT|SC_MOD_DELETETEXT)){
			if(compile_on_modify){
				compile_on_modify=FALSE;
				compile_program();
				compile_on_modify=TRUE;
			}
		}
		break;
	default:
		return;
	}
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	printf("message=%i\n",Message);
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
