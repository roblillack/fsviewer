#ifndef _FSUTILS_H_
#define _FSUTILS_H_

#define   MAX_LEN 512

void      FSInitSystemInfo(FSViewer *fsV);
char     *FSProcessor();
char     *FSSystemRelease();
char     *FSMemory();
char     *FSDisk();
char     *FSNodeName();
void      FSLaunchApp(FSViewer *fsViewer, AppEvent event);
void      FSSetButtonImageFromFile(WMButton *btn, char *imgName);
void      FSSetButtonImageFromXPMData(WMButton *btn, char **data);
void      FSLoadIconPaths(WMList *list);
WMPixmap *FSMakePixmap(WMScreen *sPtr, char **data, int width, int height);
void      FSErrorDialog(char *title, char *msg);
int       FSConfirmationDialog(char *title, char *msg);
mode_t    FSGetUMask();
void	  FSUpdateFileView();
char     *FSGetHomeDir();
int       FSStringMatch(char *pattern, char *fn);
WMPixmap *FSCreateBlendedPixmapFromFile(WMScreen *scr, char *fileName, 
					RColor *color);
WMPixmap *FSCreatePixmapWithBackingFromFile(WMScreen *scr, char *fileName, 
					    RColor *color);
WMPixmap *FSCreateBlurredPixmapFromFile(WMScreen *scr, char *fileName);
void      LaunchApp(FSViewer *fsViewer, FileInfo *fileInfo, AppEvent event);
char     *FSParseCmdField(FileInfo *fileInfo, char *txt, ...);
char     *FSParseExecString( char *pathname, char *txt, ...);
void      FSUnsetCursor(WMWidget *w);
void      FSSetCursor(WMWidget *w, char *type, char *name);
void      FSSetBusyCursor(WMWidget *w, Bool state);
int       FSGetDNDType(FileInfo *fileInfo);
int       FSExecCommand(char *path, char *execStr);
Bool      FSImageTypeIsSupported(char *imgType);

typedef struct _DISC
{
    FSViewer *app;
    /* Nickname for mount point */
    char     *name;
    /* Mount point */
    char     *point;
    /* Device name */
    char     *device;
    /* Device name */
    char     *mntCmd;
    /* Device name */
    char     *umntCmd;
    /* Device name */
    char     *ejectCmd;
    /* Device name */
    char     *closeCmd;
} _DISC;

typedef struct _DISC Disk;

#endif
