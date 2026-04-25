cmake_minimum_required(VERSION 3.31)
project(prolog_interp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(prolog_interp_lib STATIC
        lexer.cpp
        lexer.h
        helper.h
        parser.cpp
        parser.h
        unification_environment.cpp
        unification_environment.h
        prolog_types.cpp
        prolog_types.h
        frontend_types.cpp
        frontend_types.h
        frame_solver.cpp
        frame_solver.h
        frame_solver_types.h)

add_executable(prolog_interp main.cpp)
target_link_libraries(prolog_interp PRIVATE prolog_interp_lib)
