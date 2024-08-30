/*
	This is free and unencumbered software released into the public domain.

	Anyone is free to copy, modify, publish, use, compile, sell, or distribute
	this software, either in source code form or as a compiled binary, for any
	purpose, commercial or non-commercial, and by any means.

	In jurisdictions that recognize copyright laws, the author or authors of
	this software dedicate any and all copyright interest in the software to
	the public domain. We make this dedication for the benefit of the public
	at large and to the detriment of our heirs and successors. We intend this
	dedication to be an overt act of relinquishment in perpetuity of all present
	and future rights to this software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
	IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	For more information, please refer to <http://unlicense.org/>
*/

/* -----------------------------------------------------------------------------
	SYNTAX EXAMPLE
----------------------------------------------------------------------------- */
/*
	defer {
		// code here
	};

	Will defer the execution of the code until the end of the scope!

	Ex:
	{
		defer {
			std::cout << "This line will be displayed second" << std::endl;
		};
		std::cout << "This line will be displayed first" << std::endl;
	}
*/

// Defer (https://stackoverflow.com/questions/32432450/what-is-standard-defer-finalizer-implementation-in-c && https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/)
#ifndef defer
#include <concepts>
#include <utility>
namespace detail {
	template <typename F>
	struct DeferImpl {
		F f;
		DeferImpl(F f) : f(f) {}
		~DeferImpl() { f(); }
	};
	struct ___defer_dummy___ {};
	template <std::invocable F> DeferImpl<F> operator<<(___defer_dummy___, F f) { return {std::move(f)}; }
}
#define DEFER_1(LINE) zz_defer##LINE
#define DEFER_2(LINE) DEFER_1(LINE)
#define DEFER_3 auto DEFER_2(__LINE__) = ::detail::___defer_dummy___{} << [&]()
#define defer DEFER_3
#endif // defer