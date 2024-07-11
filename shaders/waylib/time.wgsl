R"(#pragma once

struct waylib_time_data {
	since_start: f32,
	delta: f32,
	average_delta: f32,
};
@group(1) @binding(0) var<uniform> time: waylib_time_data;
)"