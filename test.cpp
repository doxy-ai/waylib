#include "waylib.hpp"
#include "window.hpp"

#include <iostream>

int main() {
	constexpr auto y = wl::vec3f(5);
	auto x = y.xx();
	// y.xy();
	std::cout << x.x << ", " << x.y << std::endl;

	auto window = wl::create_window(800, 600, "waylib");
	auto [device, surface] = wl::create_default_device_from_window(window);

	auto uncapturedErrorCallbackHandle = device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});

	auto queue = device.getQueue();

	std::cout << "Window: " << window << "\n"
		<< "Instance: " << device.getAdapter().getInstance() << "\n"
		<< "Adapter: " << device.getAdapter() << "\n"
		<< "Device: " << device << "\n"
		<< "Surface: " << surface << "\n"
		<< "Queue: " << queue << std::endl;

	while(!wl::window_should_close(window)) {}

	queue.release();
	surface.release();
	wl::release_device(device);
	wl::window_free(window);
}