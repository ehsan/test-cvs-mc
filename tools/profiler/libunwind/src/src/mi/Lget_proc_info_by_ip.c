#define UNW_LOCAL_ONLY
#include <libunwind.h>
#if defined(UNW_LOCAL_ONLY) && !defined(UNW_REMOTE_ONLY)
#include "Gget_proc_info_by_ip.c"
#endif
