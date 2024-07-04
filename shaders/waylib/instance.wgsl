R"(
#pragma once

struct waylib_model_instance_data {
	transform: mat4x4f,
	tint: vec4f,
};
@group(0) @binding(0) var<storage, read> instances: array<waylib_model_instance_data>;
)"