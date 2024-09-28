#include <iostream>

#include "waylib/waylib.hpp"

int main() {
	auto window = wl::window::create({800, 600});
	while (!window.should_close())
		wl::thread_pool::enqueue([]{
			std::cout << "Hello 世界" << std::endl;
		});
}