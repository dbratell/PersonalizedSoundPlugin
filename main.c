/**
 * TODO: Translatable - different sounds for different events - 
 * version number - release - better logging - relative paths
 */

#include <windows.h>
#include "../SDK/headers_c/newpluginapi.h"
#include "../SDK/headers_c/m_clist.h"
#include "../SDK/headers_c/m_skin.h"
#include "../SDK/headers_c/m_system.h"
#include "../SDK/headers_c/m_userinfo.h"
#include "../SDK/headers_c/m_langpack.h"
#include "../SDK/headers_c/m_database.h"
#include <stdio.h>
#include "internal.h"
#ifdef USE_POPUP
#include "popup.h"
#endif
#include "resource.h"

static const char DEFAULT_SOUND_STRING[] = "<default sound>";
static const char MODULE_NAME[] = "PersonalizedSoundPlugin";

static HINSTANCE hInst;
static PLUGINLINK *pluginLink;

//---------------------------
//---Hooks

//---Handles to my hooks, needed to unhook them again
static HANDLE hHookedInit;
static HANDLE hHookedNewEvent;
static HANDLE hHookedUserInfo;

static const PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	(char*)MODULE_NAME,
	PLUGIN_MAKE_VERSION(0,5,1,0),
	"This plugin makes it possible for each user on the list to have a special sound when a message arrives.",
	"Daniel Bratell",
	"bratell@lysator.liu.se",
	"© 2003 Daniel Bratell",
	"http://www.miranda-im.org/",
	0,		//not transient
	0		//doesn't replace anything built-in
};

static char s_mirandaDir[MAX_PATH];
static int s_mirandaDirLen;

static __inline void log0(const char* message)
{
#ifdef _DEBUG
    FILE *logfile = fopen("c:\\\\debug.log", "a");
    if (logfile)
    {
        fprintf(logfile, "%s\n", message);
        fclose(logfile);
    }
#endif
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst = hinstDLL;
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        /* Save ourselves and Miranda some thread overhead */
        DisableThreadLibraryCalls(hinstDLL);
    }
    return TRUE;
}

__declspec(dllexport) const PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
    return &pluginInfo;
}

static __inline BOOL IsAbsolutePath(const char *path)
{
    return (*path == '\\' && *(path+1) == '\\') || // \\computer
        (*path != '\0' && *(path+1) == ':'); // Volume:
}

static void MakeRelativeIfUnderMiranda(char *path)
{
    log0("MakeRelativeIfUnderMiranda path:");
    log0(path);

    if (s_mirandaDirLen == 0)
    {
        return;
    }

    if (strncmp(s_mirandaDir, path, s_mirandaDirLen) == 0)
    {
        log0("In the miranda dir");
        memmove(path, path + s_mirandaDirLen, strlen(path + s_mirandaDirLen) + 1);
        log0(path);
    }
    else
    {
        log0("Not in the miranda dir");
    }
}

/**
 * Might change |path|
 */
static void WriteContactMessageSound(HANDLE contactHandle, char *path)
{
    MakeRelativeIfUnderMiranda(path);
    DBWriteContactSettingString((HANDLE)contactHandle, MODULE_NAME ,"MessageArrived", path);
}

/**
 * Returns TRUE if it succeeds.
 */
static BOOL GetContactMessageSound(HANDLE contactHandle, char *buf, unsigned int bufSize)
{
    DBVARIANT personSound;
    int rv;
    rv = DBGetContactSetting((HANDLE)contactHandle, MODULE_NAME, "MessageArrived", &personSound);
    if (rv == 0)
    {
        if (strlen(personSound.pszVal) < bufSize - 1)
        {
            strcpy(buf, personSound.pszVal);
        }
        else
        {
            log0("Too small buffer for:");
            log0(personSound.pszVal);

            rv = 1;
        }
        DBFreeVariant(&personSound);
    }
    return rv == 0;
}

static BOOL IsSoundEnabled()
{
#if 0
    UINT nStatus = CallService(MS_CLIST_GETSTATUSMODE, 0, 0);

    switch ( nStatus )
    {
    case ID_STATUS_OFFLINE:
        return FALSE ;
    case ID_STATUS_ONLINE:
	return DBGetContactSettingByte(NULL, "NoSound" ,"Online", 1);
    case ID_STATUS_AWAY:
        return DBGetContactSettingByte(NULL, "NoSound" ,"Away", 1);
    case ID_STATUS_DND:
        return DBGetContactSettingByte(NULL, "NoSound" ,"Dnd", 1);
    case ID_STATUS_NA:
	return DBGetContactSettingByte(NULL, "NoSound" ,"Na", 1);
    case ID_STATUS_OCCUPIED:
	return DBGetContactSettingByte(NULL, "NoSound" ,"Occupied", 1);
    case ID_STATUS_FREECHAT:
	return DBGetContactSettingByte(NULL, "NoSound" ,"Freechat", 1);
    case ID_STATUS_INVISIBLE:
	return DBGetContactSettingByte(NULL, "NoSound" ,"Invisible", 1);
    }
#endif

    return TRUE ;
}

static void Dequotify(char *string)
{
    if (string[0] == '"')
    {
        char *pszQuote = strchr(string+1, '"');
	if (pszQuote)
        {
            *pszQuote='\0';
            MoveMemory(string, string + 1, pszQuote - string);
        }
    }
}

static BOOL SelectSoundFile(HANDLE hwndDlg, char *buf, int bufSize)
{
    char filter[512];
    char *pfilter = filter;
    BOOL returnValue;
    OPENFILENAME ofn;
    log0("User pressed browse...");
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndDlg;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    strcpy(pfilter, Translate("Sound files"));
    strcat(pfilter, " (*.wav)");
    pfilter += strlen(pfilter) + 1;
    strcpy(pfilter, "*.wav");
    pfilter += strlen(pfilter) + 1;
    strcpy(pfilter, Translate("All Files"));
    strcat(pfilter, " (*)");
    pfilter += strlen(pfilter) + 1;
    strcpy(pfilter, "*");
    pfilter += strlen(pfilter) + 1;
    *pfilter = '\0';
    ofn.lpstrFilter = filter;

    ofn.nMaxFile = bufSize - 2;
    ofn.lpstrFile = buf;
    GetDlgItemText(hwndDlg, IDC_FILENAME, buf, bufSize);
    log0("Previous file");
    log0(buf);
    if (strcmp(Translate(DEFAULT_SOUND_STRING), buf) == 0)
    {
        log0("Default sound");
        buf[0] = '\0';
    }
    Dequotify(buf);
    log0("Now buf is");
    log0(buf);

    ofn.nMaxFileTitle = MAX_PATH;
    log0(ofn.lpstrFile);
    returnValue = GetOpenFileName(&ofn);
#ifdef _DEBUG
    {
        char debugBuf[1024];
        sprintf(debugBuf, "Result: %d, %d", returnValue, returnValue == 0 ? CommDlgExtendedError() : 0);
        log0("result:");
        log0(debugBuf);
    }
#endif
    if (returnValue)
    {
        Dequotify(buf);
    }
    return returnValue;
}

static void PlayDefaultSound(int messageType)
{
    switch (messageType)
    {
    case EVENTTYPE_URL:
        SkinPlaySound("RecvUrl");
        break;
    case EVENTTYPE_FILE:
        SkinPlaySound("RecvFile");
        break;
    default:
        SkinPlaySound("RecvMsg");
    }
}

static void MakeAbsolutePath(char *path, unsigned int bufSize)
{
    char tempBuf[MAX_PATH+1];
    if (!IsAbsolutePath(path))
    {
        if (s_mirandaDirLen + strlen(path) + 1 > bufSize)
        {
            return;
        }
        strncpy(tempBuf, path, sizeof(tempBuf));
        strcpy(path, s_mirandaDir);
        strcat(path, tempBuf);
    }
}

static void PlayUserSound(HANDLE contactHandle, int playDefault, int fallbackToDefault, int messageType)
{
    char soundPath[MAX_PATH+1];
#ifdef _DEBUG
    const char *contactName;

    contactName = (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)contactHandle, 0);
    log0(contactName);
#endif

    if (GetContactMessageSound(contactHandle, soundPath, sizeof(soundPath)))
    {
        log0(soundPath);
        MakeAbsolutePath(soundPath, sizeof(soundPath));
	if (!PlaySound(soundPath, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT))
        {
            log0("Couldn't play sound");
            if (fallbackToDefault)
            {
                PlayDefaultSound(messageType);
            }
        }
    }
    else
    {
        log0("No personalized sound");
        if (playDefault)
        {
            PlayDefaultSound(messageType);
        }
    }
}

static void DoBrowseButton(HANDLE hwndDlg)
{
    char str[MAX_PATH+2];
    HANDLE contactHandle;

    if (!SelectSoundFile(hwndDlg, str, sizeof(str)))
    {
        return;
    }

    SetDlgItemText(hwndDlg, IDC_FILENAME, str);
    contactHandle = (HANDLE)GetDlgItemInt(hwndDlg, IDC_USERHANDLE, NULL, 0);
    WriteContactMessageSound(contactHandle, str);
    log0(str);
}

static void InsertUserData(HANDLE hwndDlg, LPARAM lParam)
{	
    HANDLE contactHandle =(HANDLE)((LPPSHNOTIFY)lParam)->lParam;
    if (contactHandle != NULL) 
    {
        char soundPath[MAX_PATH+1];
        log0("We have a log handle");
        if (GetContactMessageSound(contactHandle, soundPath, sizeof(soundPath)))
        {
            SetDlgItemText(hwndDlg, IDC_FILENAME, soundPath);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_FILENAME, Translate(DEFAULT_SOUND_STRING));
        }
        SetDlgItemInt(hwndDlg, IDC_USERHANDLE, (UINT)contactHandle, 0);
    }
}

/**
 * Looks up the sound file and if there is specified a special sound file
 * and that file can not be found, than TRUE is returned, else FALSE.
 */
static BOOL IsSoundFileMissing(HANDLE contactHandle)
{
    char soundPath[MAX_PATH+1];

    if (GetContactMessageSound((HANDLE)contactHandle, soundPath, sizeof(soundPath)))
    {
        WIN32_FIND_DATA findData;
        HANDLE findHandle;

        log0(soundPath);
        MakeAbsolutePath(soundPath, sizeof(soundPath));
        findHandle = FindFirstFile(soundPath, &findData);
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            FindClose(findHandle);
            log0("File existed");
        }
        else
        {
            log0("File was missing");
            return TRUE;
        }
    }
    else
    {
        log0("No special file");
    }

    return FALSE;
}

static INT_PTR CALLBACK UserInfoPersonSoundDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnmhdr;

    switch(msg) {
        case WM_INITDIALOG:
            TranslateDialogDefault(hwndDlg);
            return TRUE;
        case WM_NOTIFY:
            lpnmhdr = (LPNMHDR)lParam;
	    if (lpnmhdr->idFrom == 0 &&
                lpnmhdr->code == PSN_INFOCHANGED)
            {
                InsertUserData(hwndDlg, lParam);
	    }
	    break;
        case WM_COMMAND:
	    switch(LOWORD(wParam)) {
		case IDCANCEL:
		    SendMessage(GetParent(hwndDlg),msg,wParam,lParam);
		    break;
                case IDC_BROWSE_BUTTON:
                    DoBrowseButton(hwndDlg);
                    break;
                case IDC_DEFAULT_SOUND:
                {
                    HANDLE contactHandle = (HANDLE)GetDlgItemInt(hwndDlg, IDC_USERHANDLE, NULL, 0);
                    log0("User pressed default");
                    SetDlgItemText(hwndDlg, IDC_FILENAME, Translate(DEFAULT_SOUND_STRING));
                    DBDeleteContactSetting((HANDLE)contactHandle, MODULE_NAME ,"MessageArrived");
                    break;
                }
                case IDC_PREVIEW_BUTTON:
                {
                    HANDLE contactHandle = (HANDLE)GetDlgItemInt(hwndDlg, IDC_USERHANDLE, NULL, 0);
                    log0("User pressed listen");
                    PlayUserSound(contactHandle, 1, 0, EVENTTYPE_MESSAGE);
                    break;
                }
	    }
	    break;
        case WM_CTLCOLORSTATIC:
            log0("Got WM_CTLCOLORSTATIC");
            //  WPARAM wParam,   // handle to DC (HDC)
            //  LPARAM lParam    // handle to static control (HWND)
            {
                HWND control = (HWND)lParam;
                if (GetDlgCtrlID(control) == IDC_FILENAME)
                {
                    HANDLE contactHandle = (HANDLE)GetDlgItemInt(hwndDlg, IDC_USERHANDLE, NULL, 0);
                    log0("for IDC_FILENAME");
                    // This is really too expensive to do here, but where else?
                    if (IsSoundFileMissing(contactHandle))
                    {
                        HDC hDC = (HDC)wParam;
                        HBRUSH redBrush;
                        log0("File didn't exist");
                        SetBkMode(hDC, TRANSPARENT);
                        SetTextColor(hDC, RGB(255,0,0)); // XXX breaks with red background
                        redBrush = GetSysColorBrush(COLOR_3DFACE);
                        return (INT_PTR) redBrush;
                    }
                }
            }
            break;
         default:
            ; // Nothing
    }
    return FALSE;
}

//---Called when a new event is added to the database. This the main operating
// function.
static int HookedUserInfo(WPARAM addInfo, LPARAM contactHandle)
//wParam=addInfo
//lParam=(LPARAM)hContact
{
    OPTIONSDIALOGPAGE odp;
//    char *szProto;
    log0("Entering HookedUserInfo");

    if (contactHandle == 0)
    {
        return 0;
    }

    log0("contactHandle != 0");

//    szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, lParam, 0);
//    if (szProto == NULL)
//		return 0;

    odp.cbSize = sizeof(odp);
    odp.hIcon = NULL;
    odp.hInstance = hInst;
    odp.pfnDlgProc = UserInfoPersonSoundDlgProc;
    odp.position = -100000;
    odp.pszTemplate = MAKEINTRESOURCE(IDD_USERINFO_PERSONSOUND);
    odp.pszTitle = "Event sounds";
    CallService(MS_USERINFO_ADDPAGE, addInfo, (LPARAM)&odp);
    return 0;
}
//---Called when a new event is added to the database. This the main operating
// function.
static int HookedNewEvent(WPARAM contactHandle, LPARAM dbEventHandle)
//wParam: contact-handle
//lParam: dbevent-handle
{
    DBEVENTINFO dbe;

    //get DBEVENTINFO without pBlob
    dbe.cbSize = sizeof(dbe);
    dbe.cbBlob = 0;
    dbe.pBlob = NULL;
    CallService(MS_DB_EVENT_GET, (WPARAM)dbEventHandle, (LPARAM)&dbe);

    // Is it an event sent by the user? -> ignore
    if (dbe.flags & DBEF_SENT)
        return 0; 

    if (IsSoundEnabled())
    {
        PlayUserSound((HANDLE)contactHandle, 0, 1, dbe.eventType);
    }
#ifdef USE_POPUP
    //which status do we have, are we allowed to post popups?

    //now finally show a plugin
    PopupShow((HANDLE)contactHandle, (HANDLE)dbEventHandle, dbe.eventType);
#endif

    return 0;
}

//---Called by the hook when all the modules are loaded
static int HookedInit(WPARAM wParam, LPARAM lParam)
{
    hHookedNewEvent = HookEvent(ME_DB_EVENT_ADDED, HookedNewEvent);
    hHookedUserInfo = HookEvent(ME_USERINFO_INITIALISE, HookedUserInfo);

    return 0;
}

static __inline void InitMirandaDir()
{
   s_mirandaDirLen = GetCurrentDirectory(MAX_PATH, s_mirandaDir);
    if (s_mirandaDirLen == 0)
    {
        log0("No current dir?");
        *s_mirandaDir = '\0';
    }
    else
    {
        log0("mirandadir:");
        log0(s_mirandaDir);
        if (s_mirandaDir[s_mirandaDirLen-1] != '\\')
        {
            s_mirandaDir[s_mirandaDirLen++] = '\\';
            s_mirandaDir[s_mirandaDirLen] = '\0';
        }
    }
 }

int __declspec(dllexport) Load(PLUGINLINK *link)
{
    log0("Entering Load");
    pluginLink = link;

    InitMirandaDir();

    // we do our initialization when all modules have loaded so let's wait for
    // that event
    hHookedInit = HookEvent(ME_SYSTEM_MODULESLOADED, HookedInit);
    return 0;
}

int __declspec(dllexport) Unload(void)
{
    UnhookEvent(hHookedInit);
    UnhookEvent(hHookedNewEvent);
    UnhookEvent(hHookedUserInfo);
    return 0;
}