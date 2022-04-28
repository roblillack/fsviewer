#ifndef _DND_H_
#define _DND_H_

#include <WINGs/WINGs.h>

WMDragSourceProcs* CreateDragSourceProcs();
WMDragDestinationProcs* CreateDragDestinationProcs();
WMArray* SupportedDataTypes();

#endif
