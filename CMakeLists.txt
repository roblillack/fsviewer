cmake_minimum_required(VERSION 3.10)

project(FSViewer VERSION 0.2.7 LANGUAGES C)
set(EXECUTABLE ${PROJECT_NAME})

find_package(PkgConfig REQUIRED)

file(GLOB REGEXP "src/regexp/*.c")
file(GLOB SRC "src/*.c")
add_executable(${EXECUTABLE} ${SRC} ${REGEXP})

pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET wrlib wmlib WUtil WINGs xpm xmu x11)

find_package(Intl)

if(Intl_FOUND)
    add_definitions(-DENABLE_NLS=1)

    file(GLOB GMO_FILES "po/*.gmo")

    foreach(_gmoFile ${GMO_FILES})
        get_filename_component(_absFile ${_gmoFile} ABSOLUTE)
        get_filename_component(_abs_PATH ${_absFile} PATH)
        get_filename_component(_lang ${_absFile} NAME_WE)

        install(FILES ${_gmoFile} DESTINATION share/locale/${_lang}/LC_MESSAGES RENAME "FSViewer.mo")
    endforeach()
endif()

# Create a "translations" target to compile translation files,
# if gettext is found
# find_package(Gettext)
# if(GETTEXT_FOUND)
# file(GLOB PO_FILES "po/*.po")
# GETTEXT_PROCESS_POT_FILE("po/FSViewer.pot" LANGUAGES de sv ru)
# endif()
target_link_libraries(${EXECUTABLE} PUBLIC PkgConfig::DEPS ${Intl_LIBRARIES} -lm)

add_compile_options(-Wall -Wextra -pedantic -Werror)
add_definitions(-DCLK=1)
add_definitions(-DPACKAGE_NAME="${PROJECT_NAME}")
add_definitions(-DVERSION="${PROJECT_VERSION}")
add_definitions(-DICONDIR="${CMAKE_INSTALL_PREFIX}/GNUstep/Apps/FSViewer.app")
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")

install(TARGETS ${EXECUTABLE} DESTINATION bin)
install(DIRECTORY xpm DESTINATION GNUstep/Apps/FSViewer.app)
install(DIRECTORY png DESTINATION GNUstep/Apps/FSViewer.app)

# add_subdirectory(src/regexp)
# add_subdirectory(src)
# add_subdirectory(defs)
