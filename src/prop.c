/* prop.c */

#include "FSViewer.h"


WMPropList*
GetDictObject(WMPropList* dictKey, WMPropList* valKey)
{
    WMPropList* dict = NULL;
    WMPropList* val  = NULL;

    if(dictKey && WMIsPLString(dictKey))
    {
	dict = WMGetFromPLDictionary(filesDB, dictKey);
	if(dict && WMIsPLDictionary(dict))
	{
	    if(valKey && WMIsPLString(valKey))
	    {
		val = WMGetFromPLDictionary(dict, valKey);
		if(val)
		    return val;
	    }
/* 	    else */
/* 		return dict; */
	}
    }

    return NULL;
} 

WMPropList*
GetCmdForExtn(char *extn, char *cmd)
{
    WMPropList* dictKey = WMCreatePLString(extn);
    WMPropList* cmdKey  = WMCreatePLString(cmd);
    WMPropList* val     = NULL;


    val = GetDictObject(dictKey, cmdKey);

    if(dictKey)
	WMReleasePropList(dictKey);
    if(cmdKey)
	WMReleasePropList(cmdKey);

    return val;
}

char *
GetExecStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "exec");
    
    if(val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetViewerStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "viewer");
    
    if(val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetEditorStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "editor");
    
    if(val)
	return WMGetFromPLString(val);
    else
	return NULL;
}

char *
GetIconStringForExtn(char *extn)
{
    WMPropList* val = NULL;
    
    val = GetCmdForExtn(extn, "icon");
    
    if(val != NULL)
	return WMGetFromPLString(val);
    else
	return NULL;
}

WMPropList*
FSRemoveArrayElement(WMPropList* array, WMPropList* val)
{
    int i;
    Bool notFound = True;

    if (array && WMIsPLArray(array)) 
    {
	for(i = 0; i < WMGetPropListItemCount(array); i++)
	{
	    if(WMIsPropListEqualTo(val, WMGetFromPLArray(array, i)))
	    {
		WMDeleteFromPLArray(array, i);
		WMReleasePropList(val);
		break;
	    }
	}
    }

    return array;
}

int
FSGetIntegerForName(char *name)
{
    return WMGetUDIntegerForKey(defaultsDB, name);
}   

void 
FSSetIntegerForName(char *name, int val)
{
    WMSetUDIntegerForKey(defaultsDB, val, name);
}   

void
FSSetStringForName(char *name, char *str)
{
    WMSetUDStringForKey(defaultsDB, str, name);
}

char *
FSGetStringForName(char *name)
{
    char *str;

    str = WMGetUDStringForKey(defaultsDB, name);

    if(str)
	return wstrdup(str);
    else
	return NULL;
}   

char *
FSGetStringForNameKey(char *name, char *key)
{
    WMPropList* val  = NULL;
    WMPropList* dict = NULL;

    dict = FSGetUDObjectForKey(defaultsDB, name);
    val  = FSGetDBObjectForKey(dict, key);

    if (!val)
        return NULL;

    if (!WMIsPLString(val))
        return NULL;

    return wstrdup(WMGetFromPLString(val));
}   

WMPropList* 
FSGetDBObjectForKey(WMPropList* dict, char *key)
{
    WMPropList* val    = NULL;
    WMPropList* valKey = WMCreatePLString(key);

    if(dict && WMIsPLDictionary(dict))
    {
	val = WMGetFromPLDictionary(dict, valKey);
    }    
    WMReleasePropList(valKey);

    return val;
}

void
FSSetStringForNameKey(char *name, char *dictKey, char *str)
{
    WMPropList* val  = NULL;
    WMPropList* key  = NULL;
    WMPropList* dict = NULL;

    val = WMCreatePLString(str);
    key = WMCreatePLString(dictKey);

    dict = FSGetUDObjectForKey(defaultsDB, name);
    if(dict)
	WMPutInPLDictionary(dict, key, val);
    else
	dict = WMCreatePLDictionary(key, val, NULL);

    WMSetUDObjectForKey(defaultsDB, dict, name);
}   

WMPropList*
FSGetArrayForNameKey(char *name, char *key)
{
    WMPropList* val  = NULL;
    WMPropList* dict = NULL;

    dict = FSGetUDObjectForKey(defaultsDB, name);
    val  = FSGetDBObjectForKey(dict, key);

    if (!val)
        return NULL;

    if (!WMIsPLArray(val))
        return NULL;

    return val;
}

Bool
InsertArrayElement(WMPropList* array, WMPropList* val)
{
    int i;
    Bool notFound = True;

    if (array && WMIsPLArray(array)) 
    {
	for(i = 0; i < WMGetPropListItemCount(array); i++)
	{
	    if(WMIsPropListEqualTo(val, WMGetFromPLArray(array, i)))
	    {
		notFound = False;
		break;
	    }
	}
	if(notFound)
	     WMAddToPLArray(array, val);
    }
    else
	array = WMCreatePLArray(val, NULL);

    return notFound;
}

WMPropList*
FSGetUDObjectForKey(WMUserDefaults *database, char *defaultName)
{
    WMPropList* origObj = NULL;
    WMPropList* copyObj = NULL;
    
    origObj = WMGetUDObjectForKey(database, defaultName);
    if(origObj) 
	copyObj = WMDeepCopyPropList(origObj);
    
    return copyObj;
}

void
InitFilesDB(FSViewer *fsViewer)
{
    WMPropList* key   = NULL;
    WMPropList* val   = NULL;
    WMPropList* dict  = NULL;
    WMPropList* array = NULL;

    /* App settings */
    WMSetUDStringForKey(defaultsDB, "FSViewer", "App");
    /* ROOT Settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("mycomputer");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "ROOT");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* Default Folder setting */
    WMSetUDStringForKey(defaultsDB, "folder", "DIRECTORY");
    /* 
       Default Icon Directory 
       This is one of the place where the app looks for
       icons.  Check config.h for value.
    */
    WMSetUDStringForKey(defaultsDB, ICONDIR, "ICONDIR");
    /* Default File setting */
    WMSetUDStringForKey(defaultsDB, "file-plain", "DEFAULT_IMG");
    /* HOME setting */
    WMSetUDStringForKey(defaultsDB, "home", "HOME");

    /* Shelf Settings */
    array = WMCreatePLArray(WMCreatePLString(wgethomedir()),
				    NULL);
    WMSetUDObjectForKey(defaultsDB, array, "SHELF");

    WMReleasePropList(array);

    /* CONSOLE setting */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xterm");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "CONSOLE");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* PROCRESS setting */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xterm -exec top");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "PROCESS");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* DISKS setting */
/*     key = WMCreatePLString("devices"); */
    val = WMCreatePLArray( WMCreatePLString("CDROM"), 
				   WMCreatePLString("/cdrom"), 
				   WMCreatePLString("/dev/hdc"),
				   WMCreatePLString("mount %s"),
				   WMCreatePLString("umount %s"),
				   WMCreatePLString("eject %s"),
				   WMCreatePLString("close %s"),
				   NULL );
    array = WMCreatePLArray( val, NULL );
    WMSetUDObjectForKey(defaultsDB, array, "DISCS");

    WMReleasePropList(array);
    WMReleasePropList(val);

    /* Magic File file */
    WMSetUDStringForKey(defaultsDB, "MagicFiles", "MAGICFILE");
   
    /* Default Editor */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xedit %s");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "MAGICASCII");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* Default Image Viewer */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xv %s");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "MAGICIMAGE");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* Default Postscript Viewer */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("ghostview %s");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, "MAGICPS");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    WMSetUDIntegerForKey(defaultsDB, 0, "DisplayDotFiles");
    WMSetUDIntegerForKey(defaultsDB, 0, "DisplayMCListPixmap");
    WMSetUDIntegerForKey(defaultsDB, 1, "DisplaySVBG");
    WMSetUDIntegerForKey(defaultsDB, 0, "SortOrder");
    WMSetUDIntegerForKey(defaultsDB, 0, "SortDisplay");
    WMSetUDIntegerForKey(defaultsDB, 169, "ColumnWidth");
    WMSetUDIntegerForKey(defaultsDB, 164, "XPOS");
    WMSetUDIntegerForKey(defaultsDB, 164, "YPOS");
    WMSetUDIntegerForKey(defaultsDB, 520, "WSIZE");
    WMSetUDIntegerForKey(defaultsDB, 390, "HSIZE");


    /* List of EXEs */
    array = WMCreatePLArray( WMCreatePLString("xemacs"),
				     WMCreatePLString("netscape"),
				     WMCreatePLString("xv"),
				     WMCreatePLString("gimp"),
				     WMCreatePLString("xedit"),
				     NULL );
    WMSetUDObjectForKey(defaultsDB, array, "EXE");
    WMReleasePropList(array);

    /* List of EXTNs */
    array = WMCreatePLArray( WMCreatePLString(".bmp"),
				     WMCreatePLString(".bz2"),
				     WMCreatePLString(".c"),
				     WMCreatePLString(".cgi"),
				     WMCreatePLString(".class"),
				     WMCreatePLString(".core"),
				     WMCreatePLString(".csh"),
				     WMCreatePLString(".dat"),
				     WMCreatePLString(".gif"),
				     WMCreatePLString(".gz"),
				     WMCreatePLString(".h"),
				     WMCreatePLString(".java"),
				     WMCreatePLString(".jpg"),
				     WMCreatePLString(".mp3"),
				     WMCreatePLString(".o"),
				     WMCreatePLString(".phtml"),
				     WMCreatePLString(".pl"),
				     WMCreatePLString(".png"),
				     WMCreatePLString(".rpm"),
				     WMCreatePLString(".sh"),
				     WMCreatePLString(".shtml"),
				     WMCreatePLString(".tgz"),
				     WMCreatePLString(".tif"),
				     WMCreatePLString(".txt"),
				     WMCreatePLString(".xbm"),
				     WMCreatePLString(".xcf"),
				     WMCreatePLString(".xpm"),
				     WMCreatePLString(".zip"),
                                     WMCreatePLString(".html"),
				     NULL );
    WMSetUDObjectForKey(defaultsDB, array, "EXTN");
    WMReleasePropList(array);

    /* Netscape Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("netscape %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".html"),
				     WMCreatePLString(".shtml"),
				     WMCreatePLString(".phtml"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("netscape");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "netscape");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .html settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("netscape %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-html");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".html");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .shtml settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("netscape %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-html");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".shtml");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .phtml settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("netscape %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-html");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".phtml");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .htm settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("netscape %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-html");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".htm");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* XEmacs Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xemacs %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".html"),
				     WMCreatePLString(".shtml"),
				     WMCreatePLString(".phtml"),
				     WMCreatePLString(".pl"),
				     WMCreatePLString(".txt"),
				     WMCreatePLString(".java"),
				     WMCreatePLString(".cgi"),
				     WMCreatePLString(".sh"),
				     WMCreatePLString(".csh"),
				     WMCreatePLString(".dat"),
				     WMCreatePLString(".c"),
				     WMCreatePLString(".h"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("xemacs");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "xemacs");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .txt settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xedit %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xedit %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-txt");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".txt");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .cgi settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xedit %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-executable");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".cgi");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .h settings */
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xemacs %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-h");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".h");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .o settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-o");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".o");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .java settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-java");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".java");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .class settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-class");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".class");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .gz settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-gz");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".gz");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .tgz settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-gz");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".tgz");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .tar.gz settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-gz");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".tar.gz");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .bz2 settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-bz2");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".bz2");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .rpm settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-rpm");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".rpm");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .png settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-png");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".png");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .zip settings */
    /* Phew, 'z' at last! */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-zip");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".zip");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .tif settings */
    /* When did 't' start coming after 'z'??? */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-tif");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".tif");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .xpm settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-xpm");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".xpm");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .jpg settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-jpg");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".jpg");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .gif settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-gif");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".gif");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .bmp settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-bmp");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".bmp");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .xbm settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-xbm");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".xbm");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .xpm settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-xpm");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xv %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".xpm");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .xcf settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-xcf");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("gimp %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".xcf");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* Gimp Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("gimp %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".jpg"),
				     WMCreatePLString(".gif"),
				     WMCreatePLString(".bmp"),
				     WMCreatePLString(".xcf"),
				     WMCreatePLString(".xpm"),
				     WMCreatePLString(".xbm"),
				     WMCreatePLString(".png"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("wilber");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "gimp");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* xedit Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xedit %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".txt"),
				     WMCreatePLString(".dat"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-txt");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "xedit");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .dat settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-dat");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xedit %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".dat");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* XV Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("xv %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".jpg"),
				     WMCreatePLString(".gif"),
				     WMCreatePLString(".bmp"),
				     WMCreatePLString(".xpm"),
				     WMCreatePLString(".xbm"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("xv");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "xv");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .mp3 settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-mp3");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("mpg123 %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".mp3");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* mpg123 Settings */
    key = WMCreatePLString("exec");
    val = WMCreatePLString("mpg123 %s");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
    
    key = WMCreatePLString("extn");    
    array = WMCreatePLArray( WMCreatePLString(".mp3"),
				     NULL );
    WMPutInPLDictionary(dict, key, array);

    WMReleasePropList(key);
    WMReleasePropList(array);

    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-mp3");
    WMPutInPLDictionary(dict, key, val);

    WMSetUDObjectForKey(defaultsDB, dict, "mp3");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .sh settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-sh");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".sh");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .csh settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-csh");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".csh");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .core settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-core");
    dict = WMCreatePLDictionary(key, val, NULL);
    WMSetUDObjectForKey(defaultsDB, dict, ".core");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .c settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-c");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".c");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* ChangeLog settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-txt");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, "ChangeLog");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    /* .pl settings */
    key = WMCreatePLString("icon");
    val = WMCreatePLString("file-dot-pl");
    dict = WMCreatePLDictionary(key, val, NULL);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("editor");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);

    WMReleasePropList(key);
    WMReleasePropList(val);
   
    key = WMCreatePLString("viewer");
    val = WMCreatePLString("xemacs %s");
    WMPutInPLDictionary(dict, key, val);
    WMSetUDObjectForKey(defaultsDB, dict, ".pl");

    WMReleasePropList(key);
    WMReleasePropList(val);
    WMReleasePropList(dict);

    WMSynchronizeUserDefaults(defaultsDB);
}

