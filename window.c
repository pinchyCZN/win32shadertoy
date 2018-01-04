#define WINVER 0x500
#define _WIN32_WINNT 0x500
#include <windows.h>

int set_window_pos(HWND hwnd,int x,int y,int w,int h,int max)
{
	int result=FALSE;
	HMONITOR hmon;
	MONITORINFO mi;
	RECT rect;
	rect.left=x;
	rect.top=y;
	rect.right=x+w;
	rect.bottom=y+h;
	hmon=MonitorFromRect(&rect,MONITOR_DEFAULTTONEAREST);
	mi.cbSize=sizeof(mi);
	if(GetMonitorInfo(hmon,&mi)){
		rect=mi.rcWork;
		if((x+w)>rect.right)
			x=rect.right-w;
		if(x<rect.left)
			x=rect.left;
		if((y+h)>rect.bottom)
			y=rect.bottom-h;
		if(y<rect.top)
			y=rect.top;
		if(w>0 && h>0){
			int rw,rh;
			rw=rect.right-rect.left;
			rh=rect.bottom-rect.top;
			if(w>rw)
				w=rw;
			if(h>rh)
				h=rh;
			if(w<25)
				w=25;
			if(h<25)
				h=25;
		}
		{
			int flags=SWP_NOZORDER;
			if(max)
				flags|=SW_MAXIMIZE;
			if(w==0 || h==0)
				flags|=SWP_NOSIZE;
			SetWindowPos(hwnd,HWND_TOP,x,y,w,h,flags);
		}
		result=TRUE;
	}
	return result;
}
int restore_scroll(HWND hedit,const char *WINDOW_NAME)
{
	int x=0,y=0,a=0,b=0;
	SCROLLINFO si={0};
	get_ini_value(WINDOW_NAME,"scrollx",&x);
	get_ini_value(WINDOW_NAME,"scrolly",&y);
	get_ini_value(WINDOW_NAME,"select_start",&a);
	get_ini_value(WINDOW_NAME,"select_end",&b);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_POS;
	si.nPos=y;
	SetScrollInfo(hedit,SB_VERT,&si,TRUE);
	si.nPos=x;
	SetScrollInfo(hedit,SB_HORZ,&si,TRUE);
	SendMessage(hedit,EM_SETSEL,a,b);
	return TRUE;
}
int restore_window(HWND hwnd,const char *WINDOW_NAME)
{
	int result=FALSE;
	int x=-100,y=-100,w=0,h=0,max=0;
	get_ini_value(WINDOW_NAME,"xpos",&x);
	get_ini_value(WINDOW_NAME,"ypos",&y);
	get_ini_value(WINDOW_NAME,"width",&w);
	get_ini_value(WINDOW_NAME,"height",&h);
	get_ini_value(WINDOW_NAME,"maximized",&max);
	return set_window_pos(hwnd,x,y,w,h,max);
}
int save_window_pos(HWND hwnd,const char *WINDOW_NAME)
{
	int result=FALSE;
	RECT rect={0};
	WINDOWPLACEMENT wp;
	if(GetWindowPlacement(hwnd,&wp)!=0){
		int w,h,maximized=0;
		rect=wp.rcNormalPosition;
		if(wp.flags&WPF_RESTORETOMAXIMIZED)
			maximized=1;
		w=rect.right-rect.left;
		h=rect.bottom-rect.top;
		write_ini_value(WINDOW_NAME,"width",w);
		write_ini_value(WINDOW_NAME,"height",h);
		write_ini_value(WINDOW_NAME,"xpos",rect.left);
		write_ini_value(WINDOW_NAME,"ypos",rect.top);
		write_ini_value(WINDOW_NAME,"maximized",maximized);
		result=TRUE;
	}
	return result;
}
int save_scroll_pos(HWND hwnd,const char *WINDOW_NAME)
{
	int x=0,y=0,a=0,b=0;
	SCROLLINFO si={0};
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_POS;
	GetScrollInfo(hwnd,SB_VERT,&si);
	y=si.nPos;
	GetScrollInfo(hwnd,SB_HORZ,&si);
	x=si.nPos;
	SendMessage(hwnd,EM_GETSEL,&a,&b);
	write_ini_value(WINDOW_NAME,"scrollx",x);
	write_ini_value(WINDOW_NAME,"scrolly",y);
	write_ini_value(WINDOW_NAME,"select_start",a);
	write_ini_value(WINDOW_NAME,"select_end",b);
	return TRUE;
}
