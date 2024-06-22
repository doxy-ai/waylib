#pragma once
#include "waylib.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#include "model.h"

// WAYLIB_OPTIONAL(model) load_model(
// 	std::string_view file_path,
// 	model_process_configuration config
// #ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
// 		= default_model_process_configuration()
// #endif
// );

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif