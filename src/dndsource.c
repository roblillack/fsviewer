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

void BeganDrag(WMView* self, WMPoint* point)
{
    //wwarning("Began drag");
}

void EndedFileViewDrag(WMView* self, WMPoint* point, Bool deposited)
{
    if (deposited) {
        // Ok, successful drag. Receiver will take care.
        return;
    }

    FSFileButton* btn = (FSFileButton*)WMWidgetOfView(self);
    if (WMWidgetClass(btn) != FileButtonWidgetClass()) {
        return;
    }

    FSFileView* fView = (FSFileView*)WMGetHangedData(btn);
    if (!fView) {
        return;
    }

    //FSFileViewRemoveFileButtonFromShelf(fView, btn);
}

WMData* FetchDragData(WMView* self, char* type)
{
    //wwarning("data type %s requested", type);

    if (!self) {
        return NULL;
    }

    WMWidget* btn = WMWidgetOfView(self);
    if (!btn) {
        return NULL;
    }

    if (WMWidgetClass(btn) != FileButtonWidgetClass()) {
        return NULL;
    }

    FileInfo* fileInfo = FSGetFileButtonFileInfo(btn);
    if (!fileInfo) {
        return NULL;
    }

    const int pathLen = strlen(fileInfo->path);
    const int nameLen = strlen(fileInfo->name);
    char* buf = (char*)malloc(7 + pathLen + nameLen + 2 + 1);
    if (!buf) {
        return NULL;
    }
    strncpy(buf, "file://", 7);
    strncpy(buf + 7, fileInfo->path, pathLen);
    strncpy(buf + 7 + pathLen, fileInfo->name, nameLen);
    strncpy(buf + 7 + pathLen + nameLen, "\r\n", 2);

    return WMCreateDataWithBytesNoCopy(buf, strlen(buf), wfree);
}

static WMArray* dataTypes = NULL;
WMArray* SupportedDataTypes()
{
    if (!dataTypes) {
        dataTypes = WMCreateArray(1);
        WMSetInArray(dataTypes, 0, &("text/uri-list"));
    }
    return dataTypes;
}

WMArray* DropDataTypes(WMView* self)
{
    //wwarning("supported data types requested");
    return SupportedDataTypes();
}

Bool AcceptDropOperation(WMView* self, WMDragOperationType operation)
{
    return True;
}

WMDragOperationType WantedDropOperation(WMView* self)
{
    return WDOperationCopy;
}

static WMDragSourceProcs fileViewDragSourceProcs = {
    .acceptDropOperation = AcceptDropOperation,
    .askedOperations = NULL,
    .beganDrag = NULL,
    .dropDataTypes = DropDataTypes,
    .endedDrag = EndedFileViewDrag,
    .fetchDragData = FetchDragData,
    .wantedDropOperation = WantedDropOperation,
};

WMDragSourceProcs* FileViewDragSourceProcs()
{
    return &fileViewDragSourceProcs;
}

static WMDragSourceProcs pathViewDragSourceProcs = {
    .acceptDropOperation = AcceptDropOperation,
    .askedOperations = NULL,
    .beganDrag = BeganDrag,
    .dropDataTypes = DropDataTypes,
    .endedDrag = NULL,
    .fetchDragData = FetchDragData,
    .wantedDropOperation = WantedDropOperation,
};

WMDragSourceProcs* PathViewDragSourceProcs()
{
    return &pathViewDragSourceProcs;
}
