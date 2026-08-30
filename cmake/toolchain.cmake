if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
	include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()
