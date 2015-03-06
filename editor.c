#define WINVER 0x500
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <richedit.h>
#include "Commctrl.h"
#include "resource.h"
#include <GL/gl.h>
#include "glext.h"

LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern HINSTANCE ghinstance;

extern int pause;
extern int fragid,progid;
extern int load_preamble;
typedef struct {
	char *func;
	char *desc;
}HELP_DATA;
static HELP_DATA help[]={
	{"normalize","(genType v); normalize returns a vector with the same direction as its parameter, v, but with length 1."},
	{"mix","(genType x,genType y,genType a); mix performs a linear interpolation between x and y using a to weight between them. The return value is computed as x*(1-a)+y*a."},
	{"dot","(genType x,genType y); dot product sum x[0]*y[0]+x[1]*y[1]+..."},
	{"cross","(vec3 x,vec3 y); x[1]*y[2]-y[1]*x[2]x[2]*y[0]-y[2]*x[0]x[0]*y[1]-y[0]*x[1]"},
	{"step","(genType edge,genType x); For element i of the return value, 0.0 is returned if x[i] < edge[i], and 1.0 is returned otherwise."},
	{"length","(genType x); length returns the length of the vector. i.e., x[0]^2 + x[1]^2..."},
};
WNDPROC orig_edit=0;
int edit_busy=FALSE;

int update_status(HWND hedit,HWND hstatus)
{
	int start=0,end=0,line;
	char str[80];
	SendMessage(hedit,EM_GETSEL,&start,&end);
	line=SendMessage(hedit,EM_LINEFROMCHAR,start,0);
	sprintf(str,"%i",line+1);
	SetWindowText(hstatus,str);
	return TRUE;
}
int find_next_word(HWND hedit,char *str,int up)
{
	FINDTEXT ftext;
	int fpos,dir,pos=0,end=0;
	SendMessage(hedit,EM_GETSEL,&pos,&end);
	dir=0;
	if(up){
		ftext.chrg.cpMax=0;
		if(pos>0)
			pos--;
	}
	else{
		ftext.chrg.cpMax=-1;
		dir=FR_DOWN;
		pos++;
	}
	ftext.lpstrText=str;
	ftext.chrg.cpMin=pos;
	fpos=SendMessage(hedit,EM_FINDTEXT,dir,&ftext);
	if(fpos>0){
		SendMessage(hedit,EM_SETSEL,fpos,fpos);
		SendMessage(hedit,EM_SCROLLCARET,0,0);
	}
	return 0;
}
int seek_start(char *str,int start)
{
	int result=-1;
	int pos=start;
	char c;
	c=str[pos];
	if(isalnum(c)){
		while(pos>=0){
			c=str[pos];
			if(isalnum(c)){
				result=pos;
			}
			else
				break;
			pos--;
		};
	}
	return result;
}
int word_len(char *str)
{
	int i=0;
	while(str[i]){
		if(!isalnum(str[i]))
			break;
		i++;
	}
	return i;
}
int get_word(HWND hedit,int charpos,char *str,int size)
{
	int result=FALSE;
	int line=SendMessage(hedit,EM_EXLINEFROMCHAR,0,charpos);
	if(line>=0){
		int index=SendMessage(hedit,EM_LINEINDEX,line,0);
		if(index>=0){
			int pos=charpos-index;
			int tmp_size=0x8000;
			char *tmp=calloc(tmp_size,1);
			if(tmp && pos>=0){
				int start,count=0;
				((short*)tmp)[0]=tmp_size;
				SendMessage(hedit,EM_GETLINE,line,tmp);
				start=seek_start(tmp,pos);
				if(start>=0){
					char *s=tmp+start;
					int end=word_len(s);
					s[end]=0;
					//printf("[%s]\n(end=%i)\n",s,end);
					strncpy(str,s,size);
					str[size-1]=0;
					result=TRUE;
				}
				free(tmp);
			}
		}
	}
	return result;
}
static char search_text[80]={0};
LRESULT CALLBACK find_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND hedit=0;
	static int start=0;
	FINDTEXT ftext;
	int pos,dir,fpos;
	static int fontheight=0,timer=0,last_search_pos=0,last_dir=0,at_top=FALSE;

	switch(msg){
	case WM_INITDIALOG:
		hedit=lparam;
		if(hedit){
			int end=0;
			start=0;
			SendMessage(hedit,EM_GETSEL,&start,&end);
			if(start!=end){
				SendMessage(hedit,WM_COPY,0,0);
				SendMessage(GetDlgItem(hwnd,IDC_SEARCH_BOX),WM_PASTE,0,0);
			}
			else{
				get_word(hedit,start,search_text,sizeof(search_text));
				SetDlgItemText(hwnd,IDC_SEARCH_BOX,search_text);
			}
			{
				HDC hdc;
				hdc=GetDC(hedit);
				if(hdc!=0){
					TEXTMETRIC tm;
					if(GetTextMetrics(hdc,&tm)!=0)
						fontheight=tm.tmHeight;
					ReleaseDC(hedit,hdc);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if((GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))
				wparam=IDC_SEARCH_UP;
			else
				wparam=IDC_SEARCH_DOWN;
		case IDC_SEARCH_UP:
		case IDC_SEARCH_DOWN:
			if(LOWORD(wparam)==IDC_SEARCH_DOWN)
				dir=FR_DOWN;
			else
				dir=0;
			search_text[0]=0;
			GetWindowText(GetDlgItem(hwnd,IDC_SEARCH_BOX),search_text,sizeof(search_text));
			if(search_text[0]==0)
				break;
			if(last_search_pos==0){
				int end;
				SendMessage(hedit,EM_GETSEL,&pos,&end);
			}
			else{
				pos=last_search_pos;
				if(dir!=last_dir){
					if(dir&FR_DOWN)
						pos++;
					else
						pos--;
				}
				else if(dir&FR_DOWN)
					pos++;

			}
			ftext.lpstrText=search_text;
			ftext.chrg.cpMin=pos;
			if(dir&FR_DOWN)
				ftext.chrg.cpMax=-1;
			else
				ftext.chrg.cpMax=0;
			fpos=SendMessage(hedit,EM_FINDTEXT,dir,&ftext);
			//printf("pos=%i fpos=%i last=%i\n",pos,fpos,last_search_pos);
			if(fpos>=0){
				int line,line2,ltmp;
				RECT rect;
				POINT point;
				GetClientRect(hedit,&rect);
				//printf(" point.x=%i point.y=%i rect.bottom=%i\n",point.x,point.y,rect.bottom);
				line2=SendMessage(hedit,EM_GETFIRSTVISIBLELINE,0,0);
				ltmp=SendMessage(hedit,EM_LINEINDEX,line2,0);
				point.y=0;
				SendMessage(hedit,EM_POSFROMCHAR,&point,ltmp);
				if(point.y<0) //check if first visible line is actually visible
					line2++;
				line=SendMessage(hedit,EM_LINEFROMCHAR,fpos,0);
				point.y=0;
				SendMessage(hedit,EM_POSFROMCHAR,&point,fpos);
				if(line-line2==0){
					if(point.y<0){
						line=0;line2=1;
					}
					else if(point.y>0 && point.y>rect.bottom-fontheight){
						line=1;line2=0;
					}
				}
				else{
					if(point.y>0 && point.y<rect.bottom){
						if(point.y>=rect.bottom-fontheight){
							line=1;line2=0;
						}
						else{
							line=0;line2=0;
						}
					}

				}
				SendMessage(hedit,EM_LINESCROLL,0,line-line2);
				//determine if the text is scrolled up beyond the bottom
				if(line-line2>0){
					line=SendMessage(hedit,EM_GETLINECOUNT,0,0);
					pos=SendMessage(hedit,EM_LINEINDEX,line-1,0);
					point.y=0;
					SendMessage(hedit,EM_POSFROMCHAR,&point,pos);
					if(point.y<rect.bottom-fontheight)
						SendMessage(hedit,EM_SCROLL,SB_LINEDOWN,0);
				}
				point.y=-1;
				SendMessage(hedit,EM_POSFROMCHAR,&point,fpos+strlen(search_text));
				if(point.y>=0 && point.x>=0 &&
					point.y<=rect.bottom && point.x<=rect.right){
					//printf("x=%i y=%i h=%i\n",point.x,point.y,rect.bottom-rect.top);
					ClientToScreen(hedit,&point);
					SetWindowPos(hwnd,NULL,point.x,point.y+fontheight,0,0,SWP_NOZORDER|SWP_NOSIZE);
				}
				//printf("fpos=%i %i %i dif=%i\n",fpos,line,line2,line-line2);
				last_search_pos=fpos;
				if(fpos==0)
					at_top=TRUE;
				else
					at_top=FALSE;
			}
			else{
				POINT point={0,0};
				if(dir&FR_DOWN){
					RECT rect;
					GetWindowRect(hedit,&rect);
					point.x=rect.left;
					point.y=rect.bottom;
				}
				else{
					ClientToScreen(hedit,&point);
				}
			}
			last_dir=dir;
			break;			
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		if(hedit && last_search_pos){
			pos=last_search_pos+strlen(search_text);
			SendMessage(hedit,EM_SETSEL,pos,pos);
		}
		last_search_pos=0;
		break;
	}
	return 0;
}
LRESULT APIENTRY subclass_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_APP:
		{
			int maxlen=0x300000;
			char *buf=malloc(maxlen);
			edit_busy=TRUE;
			if(buf){
				HWND hparent=GetParent(hwnd);
				GetWindowText(hwnd,buf,maxlen);
				buf[maxlen-1]=0;
				if(load_preamble)
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
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_KEYUP:
		update_status(hwnd,GetDlgItem(GetParent(hwnd),IDC_STATUS));
		break;
	case WM_KEYDOWN:
		switch(wparam){
		case VK_TAB:
			if(GetKeyState(VK_CONTROL)&0x8000){
				SetFocus(GetDlgItem(GetParent(hwnd),IDC_SETTINGS));
				return 0;
			}
			else if(GetKeyState(VK_SHIFT)&0x8000){
				PostMessage(hwnd,WM_KEYDOWN,VK_BACK,0);
				PostMessage(hwnd,WM_KEYDOWN,VK_BACK,0);
			}
			break;
		case VK_F3:
			if(search_text[0]!=0){
				find_next_word(hwnd,search_text,GetKeyState(VK_SHIFT)&0x8000);
			}
			break;
		case 'F':
			if(GetKeyState(VK_CONTROL)&0x8000)
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FIND),hwnd,find_proc,hwnd);	
			break;
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		}
		break;
	}
	return CallWindowProc(orig_edit,hwnd,msg,wparam,lparam); 
}
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
		if(x>(rect.right-25) || x<(rect.left-25)
			|| y<(rect.top-25) || y>(rect.bottom-25))
			;
		else{
			if(w>0 && h>0){
				int flags=SWP_NOZORDER;
				if(max)
					flags|=SW_MAXIMIZE;
				SetWindowPos(hwnd,HWND_TOP,x,y,w,h,flags);
				result=TRUE;
			}
		}
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
int get_twipsx(float *x)
{
	HDC hdc;
	hdc=GetDC(HWND_DESKTOP);
	if(hdc){
		float f;
		f=GetDeviceCaps(hdc,LOGPIXELSY);
		*x=1440/f;
		ReleaseDC(HWND_DESKTOP,hdc);
	}
	return TRUE;
}

LRESULT CALLBACK WndEdit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		orig_edit=SetWindowLong(GetDlgItem(hwnd,IDC_EDIT1),GWL_WNDPROC,subclass_edit);
		SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETFONT,GetStockObject(ANSI_FIXED_FONT),0);
		SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETEVENTMASK,0,ENM_CHANGE);
		SendDlgItemMessage(hwnd,IDC_EDIT1,EM_LIMITTEXT,0,0);
		{
			float twipsx=0;
			get_twipsx(&twipsx);
			if(twipsx>0){
				int i,x;
				PARAFORMAT pf={0};
				pf.cbSize=sizeof(PARAFORMAT);
				pf.dwMask=PFM_TABSTOPS;
				x=twipsx*8*4;
				for(i=0;i<MAX_TAB_STOPS;i++){
					pf.rgxTabs[i]=x*i;
				}
				pf.cTabCount=i;
				SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETPARAFORMAT,0,&pf);
			}
		}
		break;
	case WM_HELP:
		//MessageBox(
		break;
	case WM_CLOSE:
		save_window_pos(hwnd,"EDITOR");
		save_scroll_pos(GetDlgItem(hwnd,IDC_EDIT1),"EDITOR");
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
		case IDC_SAVE:
			if((GetKeyState(VK_CONTROL)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))
				load_current(GetDlgItem(hwnd,IDC_EDIT1));
			else
				save_text(GetDlgItem(hwnd,IDC_EDIT1),GetDlgItem(hwnd,IDC_STATUS));
			break;
		case IDC_SETTINGS:
			DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),hwnd,settings_proc,GetDlgItem(hwnd,IDC_EDIT1));
			break;
		}
		break;
	case WM_SIZE:
		{
			int w,h;
			w=LOWORD(lparam);
			h=HIWORD(lparam);
			SetWindowPos(GetDlgItem(hwnd,IDC_EDIT1),NULL,0,0,w,h-25,SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_SETTINGS),NULL,0,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_PAUSE),NULL,100,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(hwnd,IDC_SAVE),NULL,200,h-25,0,0,SWP_NOSIZE|SWP_NOZORDER);
			{
				int cx,cy;
				cx=w-300;
				if(cx<0)
					cx=10;
				cy=25;
				SetWindowPos(GetDlgItem(hwnd,IDC_STATUS),NULL,300,h-25,cx,cy,SWP_NOZORDER);
			}
		}
		break;
	}
	return 0;
}