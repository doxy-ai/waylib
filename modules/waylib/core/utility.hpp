#pragma once
#include "config.hpp"
#include "thirdparty/thread_pool.hpp"
#include "optional.h"

#ifdef __cpp_exceptions
	#include <stdexcept>
#endif

WAYLIB_BEGIN_NAMESPACE

	template<typename T>
	struct result: expected<T, std::string> {
		using expected<T, std::string>::expected;
	};

	struct void_like{};
	template<>
	struct result<void>: expected<void_like, std::string> {
		using expected<void_like, std::string>::expected;
	};

	struct errors {
	protected:
		static std::string singleton;
	public:
		inline static void clear() { singleton.clear(); }
		inline static std::string get_and_clear() { return std::move(singleton); }
		inline static WAYLIB_NULLABLE(const char*) get() {
			if(singleton.empty()) return nullptr;
			return singleton.c_str();
		}
		inline static void set(const std::string_view view) { singleton = view; }
		inline static void set(const std::string& str) { singleton = str; }

		template<typename T>
		inline static void set(const result<T>& res) {
			if(res) return; // No errors no message
			set(res.error());
		}
	};

#ifdef __cpp_exceptions
	struct exception: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	inline T throw_if_error(const WAYLIB_OPTIONAL(T)& opt) {
		if(opt.has_value) return opt.value;

		auto msg = errors::get_and_clear();
		throw exception(msg.empty() ? "Null value encountered" : msg);
	}
	template<typename T>
	inline T& throw_if_error(const WAYLIB_NULLABLE(T*) opt) {
		if(opt) return *opt;

		auto msg = errors::get_and_clear();
		throw exception(msg.empty() ? "Null value encountered" : msg);
	}
	template<typename T>
	inline T throw_if_error(const result<T>& res) {
		if(res) return *res;
		throw exception(res.error());
	}
#else
	template<typename T>
	inline T throw_if_error(const WAYLIB_OPTIONAL(T)& opt) {
		assert(opt.has_value, errors::get());
		return opt.value;
	}
	template<typename T>
	inline T& throw_if_error(const WAYLIB_NULLABLE(T*) opt) {
		assert(opt, errors::get());
		return *opt;
	}
	template<typename T>
	inline T throw_if_error(const result<T>& res) {
		if(res) return *res;
		throw exception(res.error());
	}
#endif

	// Creates a c-string from a string view
	// (if the string view doesn't point to a valid cstring a temporary one that
	//		is only valid until the next time this function is called is created)
	template<size_t uniqueID = 0>
	inline const char* cstring_from_view(const std::string_view view) {
		static std::string tmp;
		if(view.data()[view.size()] == '\0') return view.data();
		tmp = view;
		return tmp.c_str();
	}

	template<typename T>
	inline WAYLIB_OPTIONAL(T) res2opt(const result<T>& res) {
		if(res) return *res;

		errors::set(res);
		return {};
	}

WAYLIB_END_NAMESPACE