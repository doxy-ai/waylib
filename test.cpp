#include <iostream>

#include "waylib/waylib.hpp"

int main() {
	wl::thread_pool::enqueue([] {
		std::cout << "Hello 世界" << std::endl;
	}).wait();
}