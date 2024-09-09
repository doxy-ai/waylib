R"(#pragma once
#include <waylib/pbr_data>
#include <waylib/light>

// Based on: https://github.com/Nadrin/PBR/blob/master/data/shaders/glsl/pbr_fs.glsl

// Constant normal incidence Fresnel factor for all dielectrics.
const Fdielectric = vec4(vec3(0.04), 1);
const PI = 3.141592;
const Epsilon = 0.00001;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
fn ndfGGX(cosLh: f32, roughness: f32) -> f32 {
	let alpha = roughness * roughness;
	let alpha2 = alpha * alpha;

	let denom = (cosLh * cosLh) * (alpha2 - 1.0) + 1.0;
	return alpha2 / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
fn gaSchlickG1(cosTheta: f32, k: f32) -> f32 {
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
fn gaSchlickGGX(cosLi: f32, cosLo: f32, roughness: f32) -> f32 {
	let r = roughness + 1.0;
	let k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Shlick's approximation of the Fresnel factor.
fn fresnelSchlick(F0: vec3f, cosTheta: f32) -> vec3f {
	return F0 + (vec3f(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

fn pbr_metalrough(light: waylib_light_data, mat: pbr_material, vert: waylib_fragment_shader_vertex) -> vec4f {
	// Outgoing light direction (vector from world-space fragment position to the "eye").
	let Lo = normalize(camera.settings3D.position - vert.position);

	// Get current fragment's normal and transform to world space.
	// vec3 N = normalize(2.0 * texture(normalTexture, vin.texcoord).rgb - 1.0);
	// N = normalize(vin.tangentBasis * N);
	let N = mat.normal;

	// Angle between surface normal and outgoing light direction.
	let cosLo = max(0.0, dot(N, Lo));

	// Specular reflection vector.
	let Lr = 2.0 * cosLo * N - Lo;

	// Fresnel reflectance at normal incidence (for metals use albedo color).
	let F0 = mix(Fdielectric, mat.color, mat.metalness).rgb;

	// Direct lighting calculation for analytical lights.
	{
		let i = 0;
		let Li = light_direction(light, vert.position);
		let Lradiance = light.color.rgb * light_intensity_attenuation(light, vert.position) * light.color.a;

		// Half-vector between Li and Lo.
		let Lh = normalize(Li + Lo);

		// Calculate angles between surface normal and various light vectors.
		let cosLi = max(0.0, dot(N, Li));
		let cosLh = max(0.0, dot(N, Lh));

		// Calculate Fresnel term for direct lighting.
		let F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Calculate normal distribution for specular BRDF.
		let D = ndfGGX(cosLh, mat.roughness);
		// Calculate geometric attenuation for specular BRDF.
		let G = gaSchlickGGX(cosLi, cosLo, mat.roughness);

		// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
		let kd = mix(vec3f(1.0) - F, vec3f(0.0), mat.metalness);

		// Lambert diffuse BRDF.
		// We don't scale by 1/PI for lighting & material units to be more convenient.
		// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
		let diffuseBRDF = kd * mat.color.rgb;

		// Cook-Torrance specular microfacet BRDF.
		let specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * cosLo);

		// Total contribution for this light.
		return vec4((diffuseBRDF + specularBRDF) * Lradiance * cosLi, 1.0);
	}
}

fn pbr_simple_ambient(light: waylib_light_data, mat: pbr_material, vert: waylib_fragment_shader_vertex) -> vec4f {
	return vec4(vec3(0.03) * mat.color.rgb * mat.ambient_occlusion, mat.color.a);
}
)"