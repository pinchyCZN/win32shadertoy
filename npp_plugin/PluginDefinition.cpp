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

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];



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
	ShaderVIEW() : DockingDlgInterface(IDD_SHADER_VIEW){};

	/*
    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
   };
   */

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};
protected :
	//virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
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
	}
	if(!settings.isCreated()){
		settings.create(&data);
		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT;
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