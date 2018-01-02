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
#include "resource.h"

static HANDLE g_hmodule=0;
static NppData nppData={0};
const int nbFunc = 2;
static FuncItem funcItem[nbFunc];

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
extern int screenw,screenh;
extern int lmb_down,clickx,clicky;
extern int pause;
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
			SetTimer(hwnd,1000,60,NULL);
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
//			Sleep(1);
//			InvalidateRect(hwnd,NULL,FALSE);
//			printf(".");
			return 0;
		}
		break;
	case WM_CLOSE:
		if(! ((GetKeyState(VK_CONTROL)&0x8000) || (GetKeyState(VK_SHIFT)&0x8000)) ){
			save_window_pos(hwnd,"MAIN_WINDOW");
		}
		exit(0);
		break;
	}
	return 0;
}


void start_shadertoy()
{
}
void show_settings()
{
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
        g_hmodule=hModule;
#ifdef _DEBUG
		open_console();
#endif
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

static bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
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
static int menu_start=0;
static int menu_settings=0;

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
    menu_start=setCommand(0, TEXT("Start shadertoy"), start_shadertoy, NULL, false);
    menu_settings=setCommand(1, TEXT("Show settings"), show_settings, NULL, false);

	HWND hview;
	hview=CreateDialog((HINSTANCE)g_hmodule,MAKEINTRESOURCE(IDD_SHADER_VIEW),NULL,(DLGPROC)WndProc);
	ShowWindow(hview,SW_SHOW);
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
