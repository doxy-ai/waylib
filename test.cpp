#include "waylib.hpp"
#include "window.hpp"

#include <iostream>

int main() {
	constexpr auto y = wl::vec3f(5);
	auto x = y.xx();
	// y.xy();
	std::cout << x.x << ", " << x.y << std::endl;

	auto window = wl::create_window(800, 600, "waylib");
	while(!wl::window_should_close(window)) {}
	wl::window_free(window);
}