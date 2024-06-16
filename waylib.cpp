#include "waylib.hpp"

#include <iostream>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif

int main() {
    constexpr auto y = vec3f(5);
    auto x = y.xx();
    // y.xy();
    std::cout << x.x << ", " << x.y << std::endl;
}