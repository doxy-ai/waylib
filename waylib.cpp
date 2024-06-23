#define WEBGPU_CPP_IMPLEMENTATION
#include "waylib.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

wgpu::Device create_default_device_from_instance(WGPUInstance instance, WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) {
	wgpu::RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	adapterOpts.powerPreference = prefer_low_power ? wgpu::PowerPreference::LowPower : wgpu::PowerPreference::HighPerformance;
	WGPUAdapter adapter = ((wgpu::instance*)&instance)->requestAdapter(adapterOpts);
	// std::cout << "Got adapter: " << adapter << std::endl;

	// std::cout << "Requesting device..." << std::endl;
	wgpu::DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	return ((wgpu::Adapter*)&adapter)->requestDevice(deviceDesc);
}

wgpu::Device create_default_device(WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) {
	return create_default_device_from_instance(wgpuCreateInstance({}), surface, prefer_low_power);
}

void release_device(WGPUDevice device, bool also_release_adapter /*= true*/, bool also_release_instance /*= true*/) {
	WGPUAdapter adapter = ((wgpu::device*)&device)->getAdapter();
	WGPUInstance instance = ((wgpu::adapter*)&adapter)->getInstance();
	wgpuDeviceRelease(device);
	if(also_release_adapter) wgpuAdapterRelease(adapter);
	if(also_release_instance) wgpuInstanceRelease(instance);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif