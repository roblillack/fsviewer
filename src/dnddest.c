#include <WINGs/WINGsP.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>

#include "FSFileButton.h"
#include "FSFileView.h"
#include "dnd.h"
#include "files.h"

void PrepareForDragOperation(WMView* self)
{
    //wwarning("DROPZONE: Prepare for drag");
}

WMArray* RequiredDataTypes(WMView* self, WMDragOperationType request,
    WMArray* sourceDataTypes)
{
    //wwarning("DROPZONE: supported data types requested");
    return SupportedDataTypes();
}

WMDragOperationType AllowedDropOperation(WMView* self,
    WMDragOperationType request,
    WMArray* sourceDataTypes)
{
    //wwarning("DROPZONE: allowed op %d? We'll answer with %d", request, WDOperationLink);

    return WDOperationLink;
}

Bool InspectDropData(WMView* self, WMArray* dropData)
{
    //wwarning("DROPZONE: Inspecting drop data");
    return True;
}

WMData* FirstNonNullDataItem(WMArray* array)
{
    WMArrayIterator iter = 0;
    WMData* data;

    WM_ITERATE_ARRAY(array, data, iter)
    {
        if (data) {
            return data;
        }
    };

    return NULL;
}

int Index(WMData* data, char ch, int startPos)
{
    const char* bytes = WMDataBytes(data);
    unsigned int len = WMGetDataLength(data);

    for (int i = startPos; i < len; i++) {
        if (*(bytes + i) == ch) {
            return i - startPos;
        }
    }

    return -1;
}

char* FindNextPath(WMData* data, int* startPos)
{
    const char* bytes = WMDataBytes(data) + *startPos;
    unsigned int len = WMGetDataLength(data) - *startPos;

    if (len < 7) {
        return NULL;
    }

    if (strncmp(bytes, "file://", 7) != 0) {
        return NULL;
    }

    int pos = Index(data, '\r', *startPos);
    if (pos < 7) {
        if ((pos = Index(data, '\n', *startPos)) < 7) {
            return NULL;
        }
    }

    char* r = (char*)wmalloc(pos - 7 + 1);
    strncpy(r, bytes + 7, pos - 7);
    r[pos - 7] = '\0';

    int next = pos + 1;
    for (int i = next; i < len; i++) {
        if (*(bytes + i) == '\r' || *(bytes + i) == '\n') {
            continue;
        }

        next = i;
        break;
    }
    *startPos += next;

    return r;
}

void PerformDragOperation(WMView* self, WMArray* dropData, WMArray* operations, WMPoint* dropLocation)
{
    //wwarning("DROPZONE: Performing drop operation");

    WMWidget* widget = WMWidgetOfView(self);
    FSFileView* fView = WMGetHangedData(widget);

    if (!fView) {
        return;
    }

    // Dropped onto the free space of the shelf
    if (WMWidgetClass(widget) == WC_Frame) {
        WMData* data = FirstNonNullDataItem(dropData);
        if (!data) {
            return;
        }

        int pos = 0;
        char* d = NULL;

        while ((d = FindNextPath(data, &pos)) != NULL) {
            //wwarning("Path sent by drag source: '%s'", d);
            FileInfo* fileInfo = FSGetFileInfo(d);
            if (fileInfo) {
                if (FSAddFileViewShelfItemIntoProplist(fView, fileInfo)) {
                    FSAddFileViewShelfItem(fView, fileInfo);
                }
            }
            wfree(d);
        }
    }
}

void ConcludeDragOperation(WMView* self)
{
    //wwarning("DROPZONE: Conclude drag");
}

static Bool IsCtrlPressed(WMView* view)
{
    Display* dpy = W_VIEW_DISPLAY(view);
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;

    if (!XQueryPointer(dpy, DefaultRootWindow(dpy),
            &root, &child, &root_x, &root_y, &win_x, &win_y, &mask)) {
        return False;
    }
    return (mask & ControlMask) != 0;
}

static FileInfo* FolderDropDestination(WMView* self)
{
    WMWidget* widget = WMWidgetOfView(self);
    if (!widget || WMWidgetClass(widget) != FileButtonWidgetClass()) {
        return NULL;
    }
    return FSGetFileButtonFileInfo(widget);
}

WMDragOperationType FolderAllowedDropOperation(WMView* self,
    WMDragOperationType request, WMArray* sourceDataTypes)
{
    FileInfo* dest = FolderDropDestination(self);
    if (!dest || !isDirectory(dest->fileType)) {
        return WDOperationNone;
    }
    return IsCtrlPressed(self) ? WDOperationCopy : WDOperationMove;
}

typedef struct DeferredFileOp {
    Bool copy;
    char* srcPath;
    char* destPath;
} DeferredFileOp;

static void runDeferredFileOp(void* clientData)
{
    DeferredFileOp* op = (DeferredFileOp*)clientData;
    FileInfo* src = FSGetFileInfo(op->srcPath);
    FileInfo* dest = FSGetFileInfo(op->destPath);

    if (src && dest) {
        if (op->copy) {
            FSCopy(src, dest);
        } else {
            FSMove(src, dest);
        }
    }

    wfree(op->srcPath);
    wfree(op->destPath);
    wfree(op);
}

void FolderPerformDragOperation(WMView* self, WMArray* dropData,
    WMArray* operations, WMPoint* dropLocation)
{
    FileInfo* dest = FolderDropDestination(self);
    if (!dest || !isDirectory(dest->fileType)) {
        return;
    }

    WMData* data = FirstNonNullDataItem(dropData);
    if (!data) {
        return;
    }

    Bool copy = IsCtrlPressed(self);
    char* destPath = GetPathnameFromPathName(dest->path, dest->name);

    int pos = 0;
    char* path = NULL;
    while ((path = FindNextPath(data, &pos)) != NULL) {
        if (strcmp(path, destPath) != 0) {
            DeferredFileOp* op = wmalloc(sizeof(DeferredFileOp));
            op->copy = copy;
            op->srcPath = wstrdup(path);
            op->destPath = wstrdup(destPath);
            WMAddIdleHandler(runDeferredFileOp, op);
        }
        wfree(path);
    }

    if (destPath)
        free(destPath);
}

static WMDragDestinationProcs* shelfDragDestinationProcs = NULL;

WMDragDestinationProcs* ShelfDragDestinationProcs()
{
    if (shelfDragDestinationProcs) {
        return shelfDragDestinationProcs;
    }

    WMDragDestinationProcs* r = wmalloc(sizeof(WMDragDestinationProcs));
    memset((void*)r, 0, sizeof(WMDragDestinationProcs));

    r->prepareForDragOperation = PrepareForDragOperation;
    r->requiredDataTypes = RequiredDataTypes;
    r->allowedOperation = AllowedDropOperation;
    r->inspectDropData = InspectDropData;
    r->performDragOperation = PerformDragOperation;
    r->concludeDragOperation = ConcludeDragOperation;

    return (shelfDragDestinationProcs = r);
}

static WMDragDestinationProcs* folderDragDestinationProcs = NULL;

WMDragDestinationProcs* FolderDragDestinationProcs()
{
    if (folderDragDestinationProcs) {
        return folderDragDestinationProcs;
    }

    WMDragDestinationProcs* r = wmalloc(sizeof(WMDragDestinationProcs));
    memset((void*)r, 0, sizeof(WMDragDestinationProcs));

    r->prepareForDragOperation = PrepareForDragOperation;
    r->requiredDataTypes = RequiredDataTypes;
    r->allowedOperation = FolderAllowedDropOperation;
    r->inspectDropData = InspectDropData;
    r->performDragOperation = FolderPerformDragOperation;
    r->concludeDragOperation = ConcludeDragOperation;

    return (folderDragDestinationProcs = r);
}
