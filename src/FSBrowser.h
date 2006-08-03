#ifndef FSBROWSER_H_
#define FSBROWSER_H_

typedef struct W_FSBrowser FSBrowser;

typedef void   FSBrowserFillColumnProc(FSBrowser *bPtr, int column);
typedef char  *FSBrowserGetItemNameProc(WMListItem *item);

extern char   *FSBrowserDidScrollNotification;
extern W_Class InitFSBrowser(WMScreen*);

FSBrowser  *FSCreateBrowser(WMWidget *parent);
void        FSSetBrowserPathSeparator(FSBrowser *bPtr, char *separator);
void        FSSetBrowserTitled(FSBrowser *bPtr, Bool flag);
void        FSLoadBrowserColumnZero(FSBrowser *bPtr);
int         FSAddBrowserColumn(FSBrowser *bPtr);
void        FSRemoveBrowserItem(FSBrowser *bPtr, int column, int row);
void        FSSetBrowserMaxVisibleColumns(FSBrowser *bPtr, int columns);
void        FSSetBrowserColumnTitle(FSBrowser *bPtr, int column, char *title);
/* WMListItem *FSAddSortedBrowserItem(FSBrowser *bPtr, int column,  */
/* 				   char *text, Bool isBranch); */
WMListItem *FSInsertBrowserItem(FSBrowser *bPtr, int column, int row, 
				char *text, Bool isBranch);
/* Don't free the returned string. */
char       *FSSetBrowserPath(FSBrowser *bPtr, char *path);
/* you can free the returned string */
char       *FSGetBrowserPath(FSBrowser *bPtr);
/* you can free the returned string */
char       *FSGetBrowserPathToColumn(FSBrowser *bPtr, int column);
void        FSSetBrowserFillColumnProc(FSBrowser *bPtr,
				       FSBrowserFillColumnProc *proc);
void        FSSetBrowserGetItemNameProc(FSBrowser *bPtr, 
					FSBrowserGetItemNameProc *proc);
void        FSSetBrowserAction(FSBrowser *bPtr, WMAction *action, 
			       void *clientData);
void        FSSetBrowserDoubleAction(FSBrowser *bPtr, WMAction *action,
				     void *clientData);
WMListItem *FSGetBrowserSelectedItemInColumn(FSBrowser *bPtr, int column);
int         FSGetBrowserFirstVisibleColumn(FSBrowser *bPtr);
int         FSGetBrowserSelectedColumn(FSBrowser *bPtr);
int         FSGetBrowserSelectedRowInColumn(FSBrowser *bPtr, int column);
int         FSGetBrowserNumberOfColumns(FSBrowser *bPtr);
WMList     *FSGetBrowserListInColumn(FSBrowser *bPtr, int column);
void        FSScrollBrowserToColumn(FSBrowser *bPtr, int column, 
				    Bool updateScroller);
void        FSSetBrowserRelief(FSBrowser *bPtr, WMReliefType relief);
void        FSSetBrowserDisplayFileCol(FSBrowser *bPtr, unsigned int display);

#endif
