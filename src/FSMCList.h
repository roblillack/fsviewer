#ifndef FSMCLIST_H_
#define FSMCLIST_H_

typedef struct _FSMCList FSMCList;

FSMCList   *FSCreateMCList(WMFrame *parent);
void        FSSetMCListPath(FSMCList *mPtr, char *path);
int         FSGetMCListNumberOfColumns(FSMCList *mPtr);
void        FSResizeMCList(FSMCList *mPtr, 
			   unsigned int width, unsigned int height);
void        FSSetMCListFilter(FSMCList *mPtr, char *filter);
char       *FSGetMCListPath(FSMCList *mPtr);
void        FSScrollMCListToColumn(FSMCList *mPtr, int col, 
				   Bool updateScroller);
void        FSLoadMCListColumnZero(FSMCList *mPtr);
char       *FSGetMCListPathToColumn(FSMCList *mPtr, int column);
void        FSSetMCListAction(FSMCList *mPtr, WMAction *action, 
			      void *clientData);
void        FSSetMCListDoubleAction(FSMCList *mPtr, WMAction *action, 
				    void *clientData);
WMListItem *FSGetMCListSelectedItemInColumn(FSMCList *mPtr, int column);
WMList     *FSGetMCListListInColumn(FSMCList *mPtr, int column);
char       *FSMCListFormatFileInfo(FSMCList *mPtr, FileInfo *fileInfo);

#endif
