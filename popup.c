#include <windows.h>
#include "../SDK/headers_c/newpluginapi.h"
#include "../SDK/headers_c/m_database.h"
#include "../SDK/headers_c/m_skin.h"
#include "../SDK/headers_c/m_clist.h"
#include "../SDK/headers_c/m_langpack.h"
#include "../Popup/m_popup.h"
#include "internal.h"
#include "popup.h"

static int PopupCount = 0;

static BOOL CALLBACK PopupDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case UM_FREEPLUGINDATA:
            PopupCount--;
	    return TRUE;
        default:
            break;
    }
    return (BOOL)DefWindowProc(hWnd, message, wParam, lParam);
}

char* GetPreview(UINT eventType, char* pBlob)
{
    char* comment1 = NULL;
    char* comment2 = NULL;
    char* commentFix = NULL;

    //now get text
    switch(eventType)
    {
	case EVENTTYPE_MESSAGE:
		if (pBlob) comment1 = pBlob;
		commentFix = Translate(POPUP_COMMENT_MESSAGE);
		break;

	    case EVENTTYPE_URL:
		    if (pBlob) comment2 = pBlob;
		    if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
		    commentFix = Translate(POPUP_COMMENT_URL);
		    break;

	    case EVENTTYPE_FILE:
		    if (pBlob) comment2 = pBlob + 4;
		    if (pBlob) comment1 = pBlob + strlen(comment2) + 5;
		    commentFix = Translate(POPUP_COMMENT_FILE);
		    break;

	    case EVENTTYPE_CONTACTS:
		commentFix = Translate(POPUP_COMMENT_CONTACTS);
		break;
	    case EVENTTYPE_ADDED:
		commentFix = Translate(POPUP_COMMENT_ADDED);
		break;
	    case EVENTTYPE_AUTHREQUEST:
		commentFix = Translate(POPUP_COMMENT_AUTH);
		break;

//blob format is:
//ASCIIZ    text, usually "Sender IP: xxx.xxx.xxx.xxx\r\n%s"
//ASCIIZ    from name
//ASCIIZ    from e-mail
//        case ICQEVENTTYPE_WEBPAGER:
//			if (pBlob) comment1 = pBlob;
////			if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
//		    commentFix = Translate(POPUP_COMMENT_WEBPAGER);
//		    break;
////blob format is:
////ASCIIZ    text, usually of the form "Subject: %s\r\n%s"
////ASCIIZ    from name
////ASCIIZ    from e-mail
//        case ICQEVENTTYPE_EMAILEXPRESS:
//			if (pBlob) comment1 = pBlob;
////			if (pBlob) comment1 = pBlob + strlen(comment2) + 1;
//		    commentFix = Translate(POPUP_COMMENT_EMAILEXP);
//		    break;

		default:
			commentFix = Translate(POPUP_COMMENT_OTHER);
   			break;
    }

    if (comment1)
        if (strlen(comment1) > 0)
                return comment1;
    if (comment2)
        if (strlen(comment2) > 0)
                return comment2;

    return commentFix;
}

int PopupShow(HANDLE hContact, HANDLE hEvent, UINT eventType)
{
    POPUPDATA pud;
    PLUGIN_DATA* pdata;
    DBEVENTINFO dbe;
    int iconID;
    char* lpzContactName;
    char* lpzText;

    //check if we should report this kind of event
    //get the prefered icon as well
    switch (eventType)
    {
        case EVENTTYPE_MESSAGE:
                iconID = SKINICON_EVENT_MESSAGE;
                break;
        case EVENTTYPE_URL:
                iconID = SKINICON_EVENT_URL;
                break;
        case EVENTTYPE_FILE:
                iconID = SKINICON_EVENT_FILE;
                break;
        default:
		iconID = SKINICON_OTHER_MIRANDA;
		break;
    }

    //there has to be a maximum number of popups shown at the same time
    if (PopupCount >= 20) // XXX
        return 2;

    //get the needed event data
    lpzContactName = (char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)hContact, 0);

    //get DBEVENTINFO with pBlob if preview is needed
    dbe.pBlob = NULL;
    dbe.cbSize = sizeof(dbe);
    dbe.cbBlob = CallService(MS_DB_EVENT_GETBLOBSIZE, (WPARAM)hEvent, 0);
    dbe.pBlob = (PBYTE)malloc(dbe.cbBlob);
    CallService(MS_DB_EVENT_GET, (WPARAM)hEvent, (LPARAM)&dbe);
    lpzText = GetPreview(eventType, dbe.pBlob);

    //set plugin_data ... will be useable within PopupDlgProc
    pdata = (PLUGIN_DATA*)malloc(sizeof(PLUGIN_DATA));
    pdata->eventType = eventType;
    pdata->hContact = hContact;
    pdata->hEvent = hEvent;

    //finally create the popup
    pud.lchContact = hContact;
    pud.lchIcon = LoadSkinnedIcon(iconID);
    strncpy(pud.lpzContactName, lpzContactName, MAX_CONTACTNAME);
    strncpy(pud.lpzText, lpzText, MAX_SECONDLINE);
    pud.colorBack = 0;
    pud.colorText = 0;
    pud.PluginWindowProc = (WNDPROC)PopupDlgProc;
    pud.PluginData = pdata;

    PopupCount++;
    CallService(MS_POPUP_ADDPOPUP, (WPARAM)&pud, 0);

    if (dbe.pBlob)
        free(dbe.pBlob);

    return 0;
}
