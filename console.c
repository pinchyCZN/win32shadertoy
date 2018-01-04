#include <windows.h>
#include <stdio.h>

#define _O_TEXT         0x4000  /* file mode is text (translated) */

HWND ghconsole=0;
int _open_osfhandle(long,int);

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
int move_console(int x,int y,int w,int h)
{
	if(ghconsole!=0)
		SetWindowPos(ghconsole,0,x,y,w,h,SWP_NOZORDER);
	return 0;
}
