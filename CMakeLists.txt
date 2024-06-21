cmake_minimum_required(VERSION 3.10)

project(FSViewer VERSION 0.2.7 LANGUAGES C)
set(EXECUTABLE ${PROJECT_NAME})

find_package(PkgConfig REQUIRED)

file(GLOB REGEXP "src/regexp/*.c")
file(GLOB SRC "src/*.c")
add_executable(${EXECUTABLE} ${SRC} ${REGEXP})

pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET wrlib wmlib WUtil WINGs xpm xmu x11)
# include_directories(${DEPS_INCLUDE_DIRS})
#find_library(LIBINTL intl)
find_package(Intl)
find_library(KVM kvm)
target_link_libraries(${EXECUTABLE} PUBLIC PkgConfig::DEPS ${Intl_LIBRARIES} ${KVM} -lm)
#if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
#  target_link_libraries(${EXECUTABLE} -lkvm)
#endif()

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_definitions(-DCLK=1)
add_definitions(-DPACKAGE_NAME="${PROJECT_NAME}")
add_definitions(-DVERSION="${PROJECT_VERSION}")
add_definitions(-DICONDIR="${CMAKE_INSTALL_PREFIX}/GNUstep/Apps/FSViewer.app")
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")

install(TARGETS ${EXECUTABLE} DESTINATION bin)
install(DIRECTORY xpm DESTINATION GNUstep/Apps/FSViewer.app)
install(DIRECTORY tiff DESTINATION GNUstep/Apps/FSViewer.app)

#add_subdirectory(src/regexp)
#add_subdirectory(src)
#add_subdirectory(defs)
