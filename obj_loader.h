#ifndef WAYLIB_OBJ_IS_AVAILABLE
#define WAYLIB_OBJ_IS_AVAILABLE

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

WAYLIB_OPTIONAL(model) load_obj_model(
	const char* file_path
);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_OBJ_IS_AVAILABLE