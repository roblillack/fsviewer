cmake_minimum_required(VERSION 3.10)

project(FSViewer VERSION 0.2.7)
set(EXECUTABLE ${PROJECT_NAME})

find_package(PkgConfig REQUIRED)

file(GLOB REGEXP "src/regexp/*.c")
file(GLOB SRC "src/*.c")
add_executable(${EXECUTABLE} ${SRC} ${REGEXP})

pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET wrlib wmlib WUtil WINGs xpm xmu x11)
# include_directories(${DEPS_INCLUDE_DIRS})
target_link_libraries(${EXECUTABLE} PUBLIC PkgConfig::DEPS -lm)

add_definitions(-DCLK=1)
add_definitions(-DPACKAGE_NAME="${PROJECT_NAME}")
add_definitions(-DVERSION="${PROJECT_VERSION}")
add_definitions(-DICONDIR="${CMAKE_INSTALL_PREFIX}/GNUstep/Apps/FSViewer.app")
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")

#add_subdirectory(src/regexp)
#add_subdirectory(src)
#add_subdirectory(defs)