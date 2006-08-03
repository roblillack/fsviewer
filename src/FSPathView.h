#ifndef FSPATHVIEW_H_
#define FSPATHVIEW_H_

typedef struct W_FSPathView FSPathView;

typedef void   FSPathViewFillColumnProc(FSPathView *bPtr, int column);

extern char   *FSPathViewDidScrollNotification;
extern W_Class InitFSPathView(WMScreen*);

FSPathView  *FSCreatePathView(WMWidget *parent);
void        FSSetPathViewPathSeparator(FSPathView *bPtr, char *separator);
void        FSLoadPathViewColumnZero(FSPathView *bPtr);
int         FSAddPathViewColumn(FSPathView *bPtr);
void        FSSetPathViewMaxVisibleColumns(FSPathView *bPtr, int columns);
;
/* Don't free the returned string. */
char       *FSSetPathViewPath(FSPathView *bPtr, char *path);
/* you can free the returned string */
char       *FSGetPathViewPath(FSPathView *bPtr);
/* you can free the returned string */
/* char       *FSGetPathViewPathToColumn(FSPathView *bPtr, int column); */
void        FSSetPathViewFillColumnProc(FSPathView *bPtr,
				       FSPathViewFillColumnProc *proc);
void        FSSetPathViewAction(FSPathView *bPtr, WMAction *action, 
			       void *clientData);
void        FSSetPathViewDoubleAction(FSPathView *bPtr, WMAction *action,
				     void *clientData);
int         FSGetPathViewFirstVisibleColumn(FSPathView *bPtr);
int         FSGetPathViewSelectedColumn(FSPathView *bPtr);
int         FSGetPathViewNumberOfColumns(FSPathView *bPtr);
/* FSFileButton *FSGetPathViewFileButtonInColumn(FSPathView *bPtr, int column); */
void        FSScrollPathViewToColumn(FSPathView *bPtr, int column, 
				    Bool updateScroller);
void        FSSetPathViewToColumn(FSPathView *pvPtr, int column);
void        FSSetPathViewColumnContents(FSPathView *pvPtr, int column, 
					char *pathname, int isBranch, 
					int backlight);

#endif
