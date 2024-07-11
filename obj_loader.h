#ifndef WAYLIB_OBJ_IS_AVAILABLE
#define WAYLIB_OBJ_IS_AVAILABLE

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

WAYLIB_OPTIONAL(model) load_obj_model(
	const char* file_path
#ifndef WAYLIB_NO_AUTOMATIC_UPLOAD
	, WAYLIB_OPTIONAL(wgpu_state) state
	#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
	#endif
#endif
);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_OBJ_IS_AVAILABLE