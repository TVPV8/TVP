cmake_minimum_required(VERSION 3.19)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project(tvp CXX)

add_subdirectory(src)
add_executable(tvp ${tvp_SRC})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_target_properties(tvp PROPERTIES CXX_STANDARD 20)
set_target_properties(tvp PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(tvp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if (NOT WIN32)
    add_compile_options(-ggdb3 -Og -Wall -Werror -pipe -fvisibility=hidden)
endif ()

set(CMAKE_CXX_FLAGS_PERFORMANCE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-fno-strict-aliasing)
endif ()

# Find packages.
find_package(OpenSSL 3.0.0 REQUIRED COMPONENTS Crypto)
find_package(fmt 6.1.2 REQUIRED)

# Look for vcpkg-provided libmariadb first
# If we link to the file directly, we might miss its dependencies from vcpkg
find_package(unofficial-libmariadb CONFIG)
if (unofficial-libmariadb_FOUND)
    set(MYSQL_CLIENT_LIBS "unofficial::libmariadb")
else ()
    find_package(MySQL REQUIRED)
endif ()

find_package(Threads REQUIRED)
find_package(PugiXML REQUIRED)

# Selects LuaJIT if user defines or auto-detected
if (DEFINED USE_LUAJIT AND NOT USE_LUAJIT)
    set(FORCE_LUAJIT ${USE_LUAJIT})
else ()
    find_package(LuaJIT)
    set(FORCE_LUAJIT ${LuaJIT_FOUND})
endif ()
option(USE_LUAJIT "Use LuaJIT" ${FORCE_LUAJIT})

if (FORCE_LUAJIT)
    if (APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000")
    endif ()
else ()
    find_package(Lua REQUIRED)
endif ()

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost 1.71.0 REQUIRED COMPONENTS date_time system iostreams)

include_directories(${Boost_INCLUDE_DIRS} ${Crypto++_INCLUDE_DIR} ${LUA_INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${PUGIXML_INCLUDE_DIR})
target_link_libraries(tvp PRIVATE
        Boost::date_time
        Boost::system
        Boost::iostreams
        fmt::fmt
        OpenSSL::Crypto
        ${CMAKE_THREAD_LIBS_INIT}
        ${Crypto++_LIBRARIES}
        ${LUA_LIBRARIES}
        ${MYSQL_CLIENT_LIBS}
        ${PUGIXML_LIBRARIES}
        )

target_link_options(tvp PUBLIC -flto=auto)

# Option to disable OTS Stats
option(ENABLE_OTS_STATISTICS "OTS Statistics" ON)
if (ENABLE_OTS_STATISTICS)
    message(STATUS "OTS Statistics enabled")
    add_definitions(-DSTATS_ENABLED)
endif()

### INTERPROCEDURAL_OPTIMIZATION ###
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT error)
if (result)
    message(STATUS "IPO / LTO enabled")
    set_target_properties(tvp PROPERTIES INTERPROCEDURAL_OPTIMIZATION True)
else ()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif ()
### END INTERPROCEDURAL_OPTIMIZATION ###

target_precompile_headers(tvp PUBLIC src/otpch.h)
