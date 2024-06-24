#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"

#include <iostream>

int main() {
	constexpr auto y = wl::vec3f(5);
	auto x = y.xx();
	// y.xy();
	std::cout << x.x << ", " << x.y << std::endl;

	{auto _model = wl::load_model("../tri.obj");
	if(!_model.has_value) return 1;
	wl::model model = _model.value;}

	{auto _image = wl::load_image("../test.png");
	if(!_image.has_value) return 2;
	wl::image image = _image.value;}
	
	{auto _image = wl::load_image("../test.exr");
	if(!_image.has_value) return 2;
	wl::image image = _image.value;}

	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_device_from_window(window);

	auto uncapturedErrorCallbackHandle = state.device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});

	wl::window_automatically_reconfigure_surface_on_resize(window, state);
	auto queue = state.device.getQueue();

	std::cout << "Window: " << window << "\n"
		<< "Instance: " << state.device.getAdapter().getInstance() << "\n"
		<< "Adapter: " << state.device.getAdapter() << "\n"
		<< "Device: " << state.device << "\n"
		<< "Surface: " << state.surface << "\n"
		<< "Queue: " << queue << std::endl;
	std::cout << state.device.getQueue() << std::endl;

	// auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
	// 	std::cout << "Queued work finished with status: " << status << std::endl;
	// };
	// wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

	wl::time time = {}; 

	while(!wl::window_should_close(window)) {
		auto frame = wl::promote_null_to_exception(
			wl::begin_drawing(state, wl::color8bit{229, 25, 51, 255})
		);
		{
			wl::time_calculations(time);
			
		}
		wl::end_drawing(state, frame);
	}

	wl::release_webgpu_state(state);
	wl::release_window(window);
}