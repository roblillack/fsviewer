#ifndef FSFILEBUTTON_H_
#define FSFILEBUTTON_H_

typedef struct W_FSFileButton FSFileButton;

FSFileButton  *FSCreateFileButton(WMWidget *parent);
FileInfo      *FSGetFileButtonFileInfo(FSFileButton *bPtr);
char          *FSGetFileButtonName(FSFileButton *bPtr);
char          *FSGetFileButtonPathname(FSFileButton *bPtr);
void           FSSetFileButtonPathname(FSFileButton *bPtr, char *pathname, 
				       int isBranch);
void           FSSetFileButtonSelected(FSFileButton *bPtr, int isSelected);
void           FSSetFileButtonAction(FSFileButton *bPtr, WMAction *action, 
				     void *clientData);
void           FSSetFileButtonDoubleAction(FSFileButton *bPtr, 
				      WMAction *doubleAction, 
				      void *doubleClientData);
void           FSGroupFileButtons(FSFileButton *bPtr, FSFileButton *newMember);
void           FSClearFileButton(FSFileButton *bPtr);

W_Class        InitFSFileButton(WMScreen *);

typedef struct FSFileIcon
{
    FSFileButton *btn;
    struct FSFileIcon *next;

} FSFileIcon;

#endif /* FSFILEBUTTON_H_ */

