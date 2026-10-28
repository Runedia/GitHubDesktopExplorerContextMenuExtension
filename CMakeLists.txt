cmake_minimum_required(VERSION 3.21)
project(GitHubDesktopExplorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
add_compile_options(/W4 /permissive- /utf-8 /EHsc /guard:cf)
add_link_options(/guard:cf /DYNAMICBASE /NXCOMPAT)

add_library(ghd_common STATIC
    src/Common/GitRepository.cpp
    src/Common/GitHubCli.cpp)
target_include_directories(ghd_common PUBLIC src/Common)

add_library(GitHubDesktopExplorer SHARED
    src/ShellExtension/DllMain.cpp
    src/ShellExtension/ClassFactory.cpp
    src/ShellExtension/GitHubDesktopCommand.cpp
    src/ShellExtension/GitHubDesktopExplorer.rc
    src/ShellExtension/GitHubDesktopExplorer.def)
target_include_directories(GitHubDesktopExplorer PRIVATE src/ShellExtension)
target_link_libraries(GitHubDesktopExplorer PRIVATE ghd_common shlwapi ole32 shell32 uuid)

add_executable(GitHubDesktopExplorerStub src/Stub/Stub.cpp)

enable_testing()
add_executable(CommonTests tests/CommonTests.cpp)
target_link_libraries(CommonTests PRIVATE ghd_common)
add_test(NAME CommonTests COMMAND CommonTests)
