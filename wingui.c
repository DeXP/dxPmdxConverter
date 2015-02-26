#include "wingui.h"

/*HWND hWnd, hDlg;*/

#if defined(_MSC_VER)
#pragma function(memset)
void *memset(void *s, int c, size_t n){
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}
int _fltused = 0;
#endif

__inline DWORD dxDeltaTime(FILETIME endTime, FILETIME startTime){
    ULARGE_INTEGER ls, lc, lr;
    lc.LowPart = endTime.dwLowDateTime;
    lc.HighPart = endTime.dwHighDateTime;
    ls.LowPart = startTime.dwLowDateTime;
    ls.HighPart = startTime.dwHighDateTime;
    lr.QuadPart = lc.QuadPart - ls.QuadPart;
    return lr.LowPart / 10000;
}

void clearLog(HWND hDlg){
    char* buf = {""};
    HWND log = GetDlgItem(hDlg, IDC_LOGRICH);
    int iOriginalLength = GetWindowTextLength(log);
    SendMessage(log, EM_SETSEL, (WPARAM)0, (LPARAM)iOriginalLength);
    SendMessage(log, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)buf);
}

void addLog(HWND hDlg, const char* str){
    HWND log = GetDlgItem(hDlg, IDC_LOGRICH);
    CHARRANGE cr;
    cr.cpMin = -1;
    cr.cpMax = -1;

    SendMessage(log, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessage(log, EM_REPLACESEL, 0, (LPARAM)str);

    SendMessage(log, WM_VSCROLL, SB_BOTTOM, 0L);
}

void dxSetState(HWND hDlg, int stateId, FILETIME startTime){
    FILETIME curTime;
    char buf[MAX_PATH];
    char buf2[MAX_PATH];

    GetSystemTimeAsFileTime(&curTime);

    LoadString(GetModuleHandle(NULL), stateId, buf2, MAX_PATH/sizeof(TCHAR));
    /* SetDlgItemText(hDlg, IDC_STATELABEL, buf2); */

    wsprintf(buf, "[%ld]\t%s\r\n", dxDeltaTime(curTime, startTime), buf2);
    addLog(hDlg, buf);
}


void dxMsgBox(HWND hDlg, UINT icon, int titleId, int textId){
    char buf[MAX_PATH];
    char title[MAX_PATH];

    LoadString(GetModuleHandle(NULL), titleId, title, MAX_PATH/sizeof(TCHAR));
    LoadString(GetModuleHandle(NULL), textId, buf, MAX_PATH/sizeof(TCHAR));
    MessageBox(hDlg, TEXT(buf), TEXT(title), icon | MB_OK);
}

BOOL FileExists(LPCTSTR fileName){
   return GetFileAttributes(fileName) != INVALID_FILE_ATTRIBUTES;
}


int guiDoConvert(HWND hDlg, const char* fileName, int format){
    char buf[MAX_PATH];
    char buf2[MAX_PATH];
    FILETIME sTime, curTime;
    unsigned int i;
	int res;
	int notExCnt = 0;
	BOOL doCheck = IsDlgButtonChecked(hDlg, IDC_ADDCHECK);
	HWND hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESSBAR);
	TPMDObj p;

    GetSystemTimeAsFileTime(&sTime);
	PmdObjInit(&p, format);
	p.precision = SendMessage(GetDlgItem(hDlg, IDC_COMBOPREC), CB_GETCURSEL, 0, 0) + 1;
	res = 0;

    clearLog(hDlg);
	SendMessage(hwndProgressBar, PBM_SETPOS, 2, 0);
	dxSetState(hDlg, IDS_STA_READING, sTime);
	res |= PmdReadFile(&p, fileName);
	SendMessage(hwndProgressBar, PBM_SETPOS, 25, 0);
    if( res < 0 ) return res;

    dxSetState(hDlg, IDS_STA_WRMATER, sTime);
	res |= ObjWriteMaterial(&p);
	SendMessage(hwndProgressBar, PBM_SETPOS, 50, 0);
	if( res < 0 ) return res;

    if( p.matErrCnt > 0 ){
        dxSetState(hDlg, IDS_STA_SPH, sTime);
        for(i=0; i< p.materialCount; i++)
            if( p.fm[i].errId != 0 ){
                /*wsprintf(buf, "%s ", p.fm[i].fileName);*/
                wsprintf(buf, "  %s\r\n", p.mat[i].fileName);
                addLog(hDlg, buf);
            }
        /* addLog(hDlg, "\r\n"); */
    }

    if( doCheck ){
        for(i=0; i< p.materialCount; i++){
            wsprintf(buf, "%s\\%s", p.outBase, p.fm[i].fileName);
            /* dxPrintf("%s\n", buf); */
            if( ! FileExists(buf) ){
                if( notExCnt == 0 ) dxSetState(hDlg, IDS_STA_MATNOTEX, sTime);
                p.fm[i].errId = 2;
                notExCnt++;
                wsprintf(buf, "  %s (#%d#)\r\n", p.fm[i].fileName, i);
                addLog(hDlg, buf);
            }
        }
        if( notExCnt > 0 ) res |= 2;
    }

    dxSetState(hDlg, IDS_STA_WRVERTEX, sTime);
    res |= ObjWriteVertex(&p);
	SendMessage(hwndProgressBar, PBM_SETPOS, 75, 0);
    if( res < 0 ) return res;

    dxSetState(hDlg, IDS_STA_WRFACES, sTime);
    res |= ObjWriteFaces(&p);
	SendMessage(hwndProgressBar, PBM_SETPOS, 100, 0);
    if( res < 0 ) return res;

    dxSetState(hDlg, IDS_STA_CONVEND, sTime);
	PmdObjFree(&p);
	GetSystemTimeAsFileTime(&curTime);

	LoadString(GetModuleHandle(NULL), IDS_TOTALTIME, buf2, MAX_PATH/sizeof(TCHAR));
	wsprintf(buf, buf2, dxDeltaTime(curTime, sTime) );
	addLog(hDlg, buf);

	return res;
}

void OnDropFiles(HWND hDlg, HDROP hDrop){
    TCHAR fileName[MAX_PATH];
    DragQueryFile(hDrop, 0, fileName, MAX_PATH);
    DragFinish(hDrop);
    SetWindowText( GetDlgItem(hDlg, IDC_INPUTEDIT), fileName );
}


INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
    OPENFILENAME ofn;
    int format = FORMAT_MQO;
    int convRes = lParam;
    char buf[MAX_PATH];
    char fileName[MAX_PATH] = "";

    HWND hwndInputEdit = GetDlgItem(hDlg, IDC_INPUTEDIT);
    HWND hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESSBAR);

    switch(uMsg){

        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case IDC_CHOOSEBUTTON:
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hDlg;
                    ofn.lpstrFilter = "PMD (*.pmd)\0*.pmd\0All (*.*)\0*.*\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST;
                    ofn.lpstrDefExt = "pmd";

                    if( GetOpenFileName(&ofn) ){
                        SetWindowText( hwndInputEdit, fileName );
                        SendMessage(hwndProgressBar, PBM_SETPOS, 0, 0);
                    }
                    break;
                case IDC_CONVERTBUTTON:
                    GetWindowText(hwndInputEdit, fileName, MAX_PATH);
                    if( IsDlgButtonChecked(hDlg, IDC_RADIOOBJ) ) format = FORMAT_OBJ;
					convRes = guiDoConvert(hDlg, fileName, format);

                    if( convRes < 0 ){
                        SendMessage(hwndProgressBar, PBM_SETPOS, 0, 0);
                        LoadString(GetModuleHandle(NULL), IDS_ERRBASE + convRes, buf, MAX_PATH/sizeof(TCHAR));
                        addLog(hDlg, buf);
                        dxMsgBox(hDlg, MB_ICONERROR, IDS_ERROR, IDS_ERRBASE + convRes);
                    } else if( convRes == 0 ) {
                        SendMessage(hwndProgressBar, PBM_SETPOS, 100, 0);
                        dxMsgBox(hDlg, MB_ICONINFORMATION, IDS_SUCCESS, IDS_SUCMSG);
                    } else {
                        /* convRes > 0 - warning */
                        SendMessage(hwndProgressBar, PBM_SETPOS, 100, 0);
                        dxMsgBox(hDlg, MB_ICONWARNING, IDS_WARNING, IDS_WARNMSG);
                    }
                    break;
                case IDC_RADIOMQO:
                    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 2, 0);
                    break;
                case IDC_RADIOOBJ:
                    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 5, 0);
                    break;
            }
        break;

        case WM_SYSCOMMAND:
            if( LOWORD(wParam) == SC_CONTEXTHELP){
                dxMsgBox(hDlg, MB_ICONINFORMATION, IDS_SUCCESS, IDS_ABOUTMSG);
                return TRUE;
            }
        break;

        case WM_DROPFILES:
            OnDropFiles(hDlg, (HDROP)wParam);
        break;

        case WM_CLOSE:
            DestroyWindow(hDlg);
            return TRUE;

        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
    }

    return FALSE;
}

int GuiCreateWindow(){
    HINSTANCE hInst = GetModuleHandle(NULL);
    HWND hDlg;
    MSG msg;
    BOOL ret;
    HICON hIcon = LoadIcon (hInst, MAKEINTRESOURCE (IDI_MYICON1));
    LPWSTR *szArgList;
    int argCount;
    int i;
    char buf[MAX_PATH];
    buf[1] = 0;

    /* InitCommonControls(); */
    LoadLibrary("RICHED32.DLL");

    hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), 0, DialogProc, 0);
    if( hDlg == NULL ){
        MessageBox(NULL, "Dialog creation failed!", "", MB_ICONERROR | MB_OK);
        return 0;
    }
    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon );
    for(i=1; i<=7; i++){
        buf[0] = '0'+i;
        SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 2, 0);
    CheckRadioButton(hDlg, IDC_RADIOMQO, IDC_RADIOOBJ, IDC_RADIOMQO);

    CheckDlgButton(hDlg, IDC_ADDCHECK, BST_CHECKED);
    LoadString(GetModuleHandle(NULL), IDS_STA_WAITFILE, buf, MAX_PATH/sizeof(TCHAR));
    addLog(hDlg, buf);
    /* ShowWindow(hDlg, SW_SHOW); */
    szArgList = CommandLineToArgvW(GetCommandLineW(), &argCount);
    if( (argCount > 1) /* && FileExists( szArgList[1] ) */ )
        SetWindowTextW( GetDlgItem(hDlg, IDC_INPUTEDIT), szArgList[1] );
    LocalFree(szArgList);

    while((ret = GetMessage(&msg, 0, 0, 0)) != 0){
        if(ret == -1)
            ExitProcess(-1);

        if(!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ExitProcess(0);
}

