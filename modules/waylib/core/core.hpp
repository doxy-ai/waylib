#pragma once

#include "config.hpp"
#include <webgpu/webgpu.hpp>

#include "utility.hpp"

WAYLIB_BEGIN_NAMESPACE

//////////////////////////////////////////////////////////////////////
// # Thread Pool
//////////////////////////////////////////////////////////////////////

	struct WAYLIB_PREFIXED(thread_pool_future) : std::future<void> {};

	struct thread_pool {
	protected:
		static ZenSepiol::ThreadPool& get_thread_pool(WAYLIB_OPTIONAL(size_t) initial_pool_size = {})
#ifdef IS_WAYLIB_CORE_CPP
		{
			static ZenSepiol::ThreadPool pool(initial_pool_size ? *initial_pool_size : std::thread::hardware_concurrency() - 1);
			return pool;
		}
#else
		;
#endif

	public:
		template <typename F, typename... Args>
		static auto enqueue(F&& function, WAYLIB_OPTIONAL(size_t) initial_pool_size = {}, Args&&... args) {
			return get_thread_pool(initial_pool_size).AddTask(function, args...);
		}
	};

	extern "C" {
		#include "core.h"
	}

WAYLIB_END_NAMESPACE