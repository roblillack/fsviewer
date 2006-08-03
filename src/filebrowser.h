#ifndef FILEBROW_H_
#define FILEBROW_H_

typedef struct W_FileBrowser FileBrowser;

FileBrowser *FSCreateFileBrowser(WMWidget *parent);

Bool         FSSetFileBrowserPath(FileBrowser *bPtr, char *path);

char        *FSGetFileBrowserPath(FileBrowser *bPtr);

void         FSSetFileBrowserAction(FileBrowser *bPtr, WMAction *action, 
				  void *clientData);

void         AddSViewButton(FileBrowser *bPtr, int column, 
			    char* text, char *imgName, Bool isBranch);

char        *FSGetFileBrowserPathToColumn(FileBrowser *bPtr, int column);

FileInfo    *FSGetFileBrowserSelectedFileInfo(FileBrowser *bPtr);
void         FSSetFileBrowserFilter(FileBrowser *bPtr, char *filter);
char        *FSGetFileBrowserFilter(FileBrowser *bPtr);
void         FSSetFileBrowserDoubleAction(FileBrowser *bPtr, 
					  WMAction *doubleAction, 
					  void *doubleClientData);
void         FSSetFileBrowserColumnWidth(FileBrowser *bPtr, int width);
void 	     FSFileBrowserKeyPress(FileBrowser *bPtr, XEvent *event);
void         FSUpdateFileBrowser(FileBrowser *bPtr, FileAction action, 
				  FileInfo *src, FileInfo *dest);
void         FSSetFileBrowserMode(FileBrowser *bPtr, int mode);

W_Class      InitFileBrowser(WMScreen*);

#endif

