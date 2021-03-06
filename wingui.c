#include "wingui.h"
#ifdef USEWINGUI

#if defined(_MSC_VER)
    /* https://gist.github.com/mmozeiko/6a365d6c483fc721b63a#file-win32_crt_float-cpp */
int _fltused = 0;
int abs(int x){
	return (x < 0 ? -x : x);
}
    #ifdef _M_IX86
    __declspec(naked) void _ftol2(){
        __asm {
            fistp qword ptr [esp-8]
            mov   edx,[esp-4]
            mov   eax,[esp-8]
            ret
        }
    }

    __declspec(naked) void _ftol2_sse(){
        __asm {
            fistp dword ptr [esp-4]
            mov   eax,[esp-4]
            ret
        }
    }

    /* http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/md/i386/_ulldiv.c.htm */
    __declspec(naked) void __cdecl _aulldiv(void)
{
    __asm {
        push    ebx
        push    esi

#define DVNDLO  [esp + 12]
#define DVNDHI  [esp + 16]
#define DVSRLO  [esp + 20]
#define DVSRHI  [esp + 24]

        mov     eax,DVSRHI      ; check to see if divisor < 4194304K
        or      eax,eax
        jnz     short L1        ; nope, gotta do this the hard way
        mov     ecx,DVSRLO      ; load divisor
        mov     eax,DVNDHI      ; load high word of dividend
        xor     edx,edx
        div     ecx             ; get high order bits of quotient
        mov     ebx,eax         ; save high bits of quotient
        mov     eax,DVNDLO      ; edx:eax <- remainder:lo word of dividend
        div     ecx             ; get low order bits of quotient
        mov     edx,ebx         ; edx:eax <- quotient hi:quotient lo
        jmp     short L2        ; restore stack and return

L1:
        mov     ecx,eax         ; ecx:ebx <- divisor
        mov     ebx,DVSRLO
        mov     edx,DVNDHI      ; edx:eax <- dividend
        mov     eax,DVNDLO
L3:
        shr     ecx,1           ; shift divisor right one bit; hi bit <- 0
        rcr     ebx,1
        shr     edx,1           ; shift dividend right one bit; hi bit <- 0
        rcr     eax,1
        or      ecx,ecx
        jnz     short L3        ; loop until divisor < 4194304K
        div     ebx             ; now divide, ignore remainder
        mov     esi,eax         ; save quotient

        mul     dword ptr DVSRHI ; QUOT * DVSRHI
        mov     ecx,eax
        mov     eax,DVSRLO
        mul     esi             ; QUOT * DVSRLO
        add     edx,ecx         ; EDX:EAX = QUOT * DVSR
        jc      short L4        ; carry means Quotient is off by 1

        cmp     edx,DVNDHI      ; compare hi words of result and original
        ja      short L4        ; if result > original, do subtract
        jb      short L5        ; if result < original, we are ok
        cmp     eax,DVNDLO      ; hi words are equal, compare lo words
        jbe     short L5        ; if less or equal we are ok, else subtract
L4:
        dec     esi             ; subtract 1 from quotient
L5:
        xor     edx,edx         ; edx:eax <- quotient
        mov     eax,esi

L2:

        pop     esi
        pop     ebx

        ret     16
    }
}

    #endif
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
    char* buf = "";
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

void addLogWithTime(HWND hDlg, const char* str, FILETIME startTime){
    FILETIME curTime;
    char buf[MAX_PATH];
    GetSystemTimeAsFileTime(&curTime);

    wsprintf(buf, "[%ld]\t%s\r\n", dxDeltaTime(curTime, startTime), str);
    addLog(hDlg, buf);
}

void dxSetState(HWND hDlg, int stateId, FILETIME startTime){
    FILETIME curTime;
    char buf[MAX_PATH];
    char buf2[MAX_PATH];

    GetSystemTimeAsFileTime(&curTime);
    LoadString(GetModuleHandle(NULL), stateId, buf2, MAX_PATH/sizeof(TCHAR));

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


int guiDoConvert(HWND hDlg, const char* fileName, int format){
    char buf[MAX_PATH];
    char buf2[MAX_PATH];
    char pngTexName[MAX_PATH];
    FILETIME sTime, curTime;
    unsigned int i, j;
    int res;
    int notExCnt = 0;
    BOOL doCheck = 1;
    BOOL texConvert = IsDlgButtonChecked(hDlg, IDC_TEXCONVERT);
    BOOL doArchive = IsDlgButtonChecked(hDlg, IDC_DOARCHIVE);
    BOOL alreadyDone = 0;
    HWND hwndProgressBar = GetDlgItem(hDlg, IDC_PROGRESSBAR);
    TPMDObj p;

    GetSystemTimeAsFileTime(&sTime);
    PmdObjInit(&p, format);
    p.precision = SendMessage(GetDlgItem(hDlg, IDC_COMBOPREC), CB_GETCURSEL, 0, 0) + 1;
    res = 0;

    clearLog(hDlg);
    SendMessage(hwndProgressBar, PBM_SETPOS, 2, 0);
    dxSetState(hDlg, IDS_STA_READING, sTime);
    res |= Read3dFile(&p, fileName);
    SendMessage(hwndProgressBar, PBM_SETPOS, 25, 0);
    if( res < 0 ) return res;

    if( texConvert ){
        dxSetState(hDlg, IDS_STA_TEXCONVERT, sTime);
        /* Convert BMP textures to PNG */
        for(i=0; i< p.materialCount; i++){
            wsprintf(buf, "%s\\%s", p.outBase, p.fm[i].fileName);
            /*dxPrintf("%s\n", buf);*/
            getPngName(p.fm[i].fileName, pngTexName);
            alreadyDone = 0;
            for(j=0; j<i; j++)
                if( dxStrCmp(pngTexName, p.fm[j].fileName) == 0 )
                    alreadyDone = 1;
            if( !alreadyDone && dxFileExists(buf) ){
                getPngName(buf, buf2);
                if( isBmpExt(buf) && (bmp2png(buf, buf2) == 0) ){
                    /* Texture convert success */
                    dxStrCpy(p.fm[i].fileName, pngTexName);

                    wsprintf(buf, "  %s", p.fm[i].fileName);
                    addLogWithTime(hDlg, buf, sTime);
                } else {
                    /* BMP to PNG convert error */
                    ;
                }
            } else if( alreadyDone ) dxStrCpy(p.fm[i].fileName, pngTexName);
            SendMessage(hwndProgressBar, PBM_SETPOS, 100*i / p.materialCount, 0);
        }

    }

    SendMessage(hwndProgressBar, PBM_SETPOS, 30, 0);
    dxSetState(hDlg, IDS_STA_WRMATER, sTime);
    res |= ObjWriteMaterial(&p);
    SendMessage(hwndProgressBar, PBM_SETPOS, 50, 0);
    if( res < 0 ) return res;

    if( p.matErrCnt > 0 ){
        dxSetState(hDlg, IDS_STA_SPH, sTime);
        for(i=0; i< p.materialCount; i++)
            if( p.fm[i].errId != 0 ){
                wsprintf(buf, "  %s\r\n", p.mat[i].fileName);
                addLog(hDlg, buf);
            }
        /* addLog(hDlg, "\r\n"); */
    }

    if( doCheck ){
        for(i=0; i< p.materialCount; i++){
            wsprintf(buf, "%s\\%s", p.outBase, p.fm[i].fileName);
            /*dxPrintf("%s\n", buf);*/
            if( ! dxFileExists(buf) ){
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

    if( doArchive ){
        Pmd2Gzip(&p);
        dxSetState(hDlg, IDS_STA_ARCHIVEEND, sTime);
    }

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

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
    char buf[MAX_PATH];
    HICON hIcon;

    switch(uMsg){
        case WM_INITDIALOG:
            hIcon = LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON1) );
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, 0);
            SendDlgItemMessage(hDlg, IDC_ICONPIC, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon );
        break;
        case WM_SETCURSOR:
            if(
               ( (HWND)wParam == GetDlgItem(hDlg, IDC_TUTLINK) ) ||
               ( (HWND)wParam == GetDlgItem(hDlg, IDC_GITLINK) )
            ){
                SetCursor(LoadCursor(NULL, IDC_HAND));
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
        break;
        case WM_CTLCOLORSTATIC:
            if(
               ( (HWND)lParam == GetDlgItem(hDlg, IDC_TUTLINK) ) ||
               ( (HWND)lParam == GetDlgItem(hDlg, IDC_GITLINK) )
            ){
                SetBkMode((HDC)wParam,TRANSPARENT);
                SetTextColor((HDC)wParam, RGB(10,10,150));
                return (INT_PTR)GetSysColorBrush(COLOR_MENU);
            }
        break;
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case IDC_AOKBUT:
                    DestroyWindow(hDlg);
                    return TRUE;
                break;
                case IDC_TUTLINK:
                case IDC_GITLINK:
                    GetWindowText( (HWND)lParam, buf, MAX_PATH / sizeof(TCHAR) );
                    ShellExecute(NULL, "open", buf, NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
            }
        case WM_CLOSE:
            DestroyWindow(hDlg);
            return TRUE;
    }
    return FALSE;
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
                    ofn.lpstrFilter = "PMD, PMX (*.pmd;*.pmx)\0*.pmd;*.pmx\0PMD (*.pmd)\0*.pmd\0PMX (*.pmx)\0*.pmx\0All (*.*)\0*.*\0";
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
                /*case IDC_RADIOMQO:
                    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 2, 0);
                    break;
                case IDC_RADIOOBJ:
                    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 5, 0);
                    break;*/
            }
        break;

        case WM_SYSCOMMAND:
            if( LOWORD(wParam) == SC_CONTEXTHELP){
                CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTDIALOG), hDlg, AboutProc, 0);
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
    SendDlgItemMessage(hDlg, IDC_COMBOPREC, CB_SETCURSEL, 5, 0);
    CheckRadioButton(hDlg, IDC_RADIOMQO, IDC_RADIOOBJ, IDC_RADIOOBJ);

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

#else
void emptyFunction(){}
#endif /* Use WinGui ? */
