/* prototypes */

int PopupShow(HANDLE hContact, HANDLE hEvent, UINT eventType);

/* constants */
#define MASK_MESSAGE    0x0001
#define MASK_URL        0x0002
#define MASK_FILE       0x0004
#define MASK_OTHER      0x0008

//---------------------------
//---Translateable Strings

#define POPUP_COMMENT_MESSAGE "Message"
#define POPUP_COMMENT_URL "URL"
#define POPUP_COMMENT_FILE "File"
#define POPUP_COMMENT_CONTACTS "Contacts"
#define POPUP_COMMENT_ADDED "You were added!"
#define POPUP_COMMENT_AUTH "Requests your authorisation"
#define POPUP_COMMENT_WEBPAGER "ICQ Web pager"
#define POPUP_COMMENT_EMAILEXP "ICQ Email express"
#define POPUP_COMMENT_OTHER "Unknown Event"
