#pragma once

struct fragment_output {
	@location(0) color: vec4f,
	@location(1) normal: vec4f,
}

struct fragment_output_with_depth {
	@location(0) color: vec4f,
	@builtin(frag_depth) depth: f32,
	@location(1) normal: vec4f,
}