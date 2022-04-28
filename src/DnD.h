#ifndef _DND_H_
#define _DND_H_

#include <WINGs/WINGs.h>

WMDragDestinationProcs* ShelfDragDestinationProcs();
WMArray* SupportedDataTypes();
WMDragSourceProcs* FileViewDragSourceProcs();
WMDragSourceProcs* PathViewDragSourceProcs();

#endif
