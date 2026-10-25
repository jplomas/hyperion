include(FetchContent)

FetchContent_Declare(
	fmtlib
	PREFIX "${PROJECT_BINARY_DIR}/deps"
	DOWNLOAD_DIR "${PROJECT_SOURCE_DIR}/deps/downloads"
	DOWNLOAD_NAME fmt-9.1.0.tar.gz
	URL https://github.com/fmtlib/fmt/archive/9.1.0.tar.gz
	URL_HASH SHA256=5dea48d1fcddc3ec571ce2058e13910a0d4a6bab4cc09a809d8b1dd1c88ae6f2
)

if (CMAKE_VERSION VERSION_LESS "3.14.0")
	FetchContent_GetProperties(fmtlib)
	if (NOT fmtlib_POPULATED)
		FetchContent_Populate(fmtlib)
		add_subdirectory(${fmtlib_SOURCE_DIR} ${fmtlib_BINARY_DIR})
	endif()
else()
	FetchContent_MakeAvailable(fmtlib)
endif()

# Newer Clang and libc++ report deprecations inside fmt 9.1.0 itself (literal operator spelling,
# std::char_traits for fmt's char8_type), which -Werror turns into errors. Treat <fmt/...> includes as
# system headers. Unlike fmt's FMT_SYSTEM_HEADERS, this keeps fmt's include directory as a normal -I,
# so the bundled fmt still wins over another fmt in a shared -isystem prefix such as Homebrew's
# /opt/homebrew/include (CIPH-HYPERION-36).
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
	target_compile_options(fmt-header-only INTERFACE --system-header-prefix=fmt/)
endif()
