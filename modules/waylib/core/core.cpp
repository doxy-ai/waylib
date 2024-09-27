#define IS_WAYLIB_CORE_CPP
#include "core.hpp"

WAYLIB_BEGIN_NAMESPACE

//////////////////////////////////////////////////////////////////////
// # Errors
//////////////////////////////////////////////////////////////////////

	std::string errors::singleton;

	WAYLIB_NULLABLE(const char*) get_error_message() {
		return errors::get();
	}

	void clear_error_message() {
		errors::clear();
	}

//////////////////////////////////////////////////////////////////////
// # Thread Pool
//////////////////////////////////////////////////////////////////////

	WAYLIB_NULLABLE(WAYLIB_PREFIXED(thread_pool_future)*) WAYLIB_PREFIXED(thread_pool_enqueue)(
		void(*function)(),
		bool return_future /*= false*/,
		WAYLIB_OPTIONAL(size_t) initial_pool_size /*= {}*/
	) {
		auto future = WAYLIB_NAMESPACE::thread_pool::enqueue<void(*)()>(std::move(function), initial_pool_size);
		if(!return_future) return nullptr;
		return new WAYLIB_PREFIXED(thread_pool_future)(std::move(future));
	}

	void WAYLIB_PREFIXED(release_thread_pool_future)(
		WAYLIB_PREFIXED(thread_pool_future)* future
	) {
		delete future;
	}

	void WAYLIB_PREFIXED(thread_pool_future_wait)(
		WAYLIB_PREFIXED(thread_pool_future)* future,
		WAYLIB_OPTIONAL(float) seconds_until_timeout /*= {}*/
	) {
		if(!seconds_until_timeout)
			future->wait();
		else {
			auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(*seconds_until_timeout));
			future->wait_for(microseconds);
		}
	}

WAYLIB_END_NAMESPACE