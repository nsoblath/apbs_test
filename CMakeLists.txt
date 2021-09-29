cmake_minimum_required (VERSION 3.16)


################################################################################
# TODO:  Add licensing and authorship information
# TODO:  Test with FEtk build
# TODO:  Handle special mac dependencies
#        (configure.ac:1306)
################################################################################
set(CMAKE_VERBOSE_MAKEFILE true)
cmake_policy(SET CMP0054 OLD)
cmake_policy(SET CMP0042 NEW)
set(CMAKE_MACOSX_RPATH 1)

################################################################################
# Set up basic project stuff                                                   #
################################################################################
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(ExternalProject)
include(FeatureSummary)
include(InstallRequiredSystemLibraries)

set(CMAKE_BUILD_TYPE "RELWITHDEBINFO")

################################################################################
#  NOTE: Overwrite the RPATH information during the install from
#  https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
################################################################################
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#include(GNUInstallDirs)
#file(RELATIVE_PATH relDir
#     ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
#     ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
#)
#set(CMAKE_INSTALL_RPATH $ORIGIN $ORIGIN/${relDir})

project("APBS_test" VERSION 1.0.0)
set(CMAKE_PROJECT_DESCRIPTION "Adaptive Poisson-Boltzmann Solver")
set(CMAKE_PROJECT_HOMEPAGE_URL "http://www.poissonboltzmann.org/")


OPTION(BUILD_SHARED_LIBS "Build shared libraries." OFF)

set(LIBRARY_INSTALL_PATH lib)
set(HEADER_INSTALL_PATH include/apbs)
set(EXECUTABLE_INSTALL_PATH bin)
set(SHARE_INSTALL_PATH share/apbs)

set(APBS_LIBS)
set(APBS_LIB_DIRS)





###############################################################################
##### Report to builder
###############################################################################
# foreach(p LIB BIN INCLUDE CMAKE)
#   file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/${INSTALL_${p}DIR} _path )
#   message(STATUS "** Installing ${p} components to ${_path}")
#   unset(_path)
# endforeach()


################################################################################
# Debian/Ubuntu hack                                                           #
################################################################################
if(EXISTS "/etc/debian_version" AND
  "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
endif()



################################################################################
# Set the lookup paths for external library dependencies                       #
################################################################################
message(STATUS "Setting lookup paths for headers and libraries")

set(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}")
list(APPEND CMAKE_INCLUDE_PATH /usr/include)
list(APPEND CMAKE_INCLUDE_PATH /usr/local/include)

find_package(Boost REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIR}")
message(STATUS "Boost include dir: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost include dir: ${Boost_LIBRARIES}")

add_executable(hello_world hello_world.cc)
target_link_libraries(hello_world ${Boost_LIBRARIES})










################################################################################
# Handle the finite element solver dependencies                                #
################################################################################
option(ENABLE_FETK "Enable the finite element solver" ON)


################################################################################
# Deal with MALOC                                                              #
# Note that we depend exclusively on the MALOC that is in our external FETK    #
# repository.  Also note, that in order for this to work on Windows, we've had #
# to add CMake support to MALOC, and that FETK has no such support, in general.#
################################################################################
# add_library(apbs_fetk_libraries INTERFACE)

set(FETK_VERSION "1.7.0" CACHE STRING "Version of FETK to use")

set(FETK_FROM_PKG TRUE)
if(NOT FETK_VERSION MATCHES "[0-9]+\.[0-9]+\.[0-9]+")
    set(FETK_FROM_PKG FALSE)
    if(NOT DEFINED FETK_FULL_URL)
        message(FATAL_ERROR "Please specify the source URL by defining: FETK_FULL_URL=[URL]")
    endif()
    string(REPLACE "/" "-" FETK_VERSION ${FETK_VERSION}) # this matches the replacement that GitHub makes for the directory within the zip file
endif()

if(ENABLE_FETK)

    set(FETK_PATH "${PROJECT_BINARY_DIR}/downloads/fetk")
    if(NOT EXISTS ${FETK_PATH})
        file(MAKE_DIRECTORY ${FETK_PATH})
    endif()

    if(FETK_FROM_PKG)

        set(FETK_BASE_URL "https://github.com/Electrostatics/FETK/releases/download/${FETK_VERSION}")

        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            set(FETK_SYSTEM_COMPONENT ${CMAKE_SYSTEM_NAME})
            set(FETK_ZIP_EXT "tar.gz")
            set(FETK_UNZIP_COMMAND "tar" )
            set(FETK_UNZIP_OPTS "-xzf")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            set(FETK_SYSTEM_COMPONENT "win64")
            set(FETK_ZIP_EXT "zip")
            set(FETK_UNZIP_COMMAND "unzip" )
        else()
            message(FATAL "System type <${CMAKE_SYSTEM_NAME}> is not supported when FETK is enabled; please disable FETK or change your system.")
        endif()

        set(FETK_NAME "FETK-${FETK_VERSION}-${FETK_SYSTEM_COMPONENT}")
        set(FETK_INSTALL_PREFIX "${FETK_PATH}/${FETK_NAME}")

        if(NOT EXISTS ${FETK_INSTALL_PREFIX}/lib)
            # download package
            set(FETK_FULL_URL "${FETK_BASE_URL}/${FETK_NAME}.${FETK_ZIP_EXT}")
            set(FETK_ZIP_FILE_NAME "${FETK_NAME}.${FETK_ZIP_EXT}")
            message(STATUS "Downloading FETK from ${FETK_FULL_URL}")
            file(DOWNLOAD ${FETK_FULL_URL} "${FETK_PATH}/${FETK_ZIP_FILE_NAME}" STATUS FETK_DL_STATUS)
            # check if download was successful
            list(GET FETK_DL_STATUS 0 FETK_DL_STATUS_VALUE)
            if(NOT FETK_DL_STATUS_VALUE EQUAL 0)
                list(GET FETK_DL_STATUS 1 FETK_DL_STATUS_STRING)
                message(FATAL_ERROR "FETK download error: ${FETK_DL_STATUS_STRING}")
            endif()

            # unzip
            message(STATUS "Unzipping: ${FETK_UNZIP_COMMAND} ${FETK_UNZIP_OPTS} ${FETK_PATH}/${FETK_ZIP_FILE_NAME}")
            execute_process(
                COMMAND ${FETK_UNZIP_COMMAND} ${FETK_UNZIP_OPTS} ${FETK_PATH}/${FETK_ZIP_FILE_NAME}
                COMMAND_ECHO STDERR
                WORKING_DIRECTORY ${FETK_PATH}
                RESULT_VARIABLE FETK_UNZIP_RETURN
            )
            if(NOT FETK_UNZIP_RETURN EQUAL 0)
                message(FATAL_ERROR "Unzip operation failed")
            endif()
        endif()

    else(FETK_FROM_PKG)

        # We assume that the user has provided FETK_FULL_URL
            
        set(FETK_ZIP_EXT "zip")
        set(FETK_UNZIP_COMMAND "unzip")

        set(FETK_NAME "FETK-${FETK_VERSION}")
        set(FETK_BUILD_PATH ${FETK_PATH}/${FETK_NAME}/build)
        set(FETK_INSTALL_PREFIX ${FETK_BUILD_PATH})

        message(STATUS "Downloading FETK from ${FETK_FULL_URL}")
        set(FETK_ZIP_FILE_NAME "${FETK_NAME}.${FETK_ZIP_EXT}")
        file(DOWNLOAD ${FETK_FULL_URL} "${FETK_PATH}/${FETK_ZIP_FILE_NAME}" STATUS FETK_DL_STATUS)
        # check if download was successful
        list(GET FETK_DL_STATUS 0 FETK_DL_STATUS_VALUE)
        if(NOT FETK_DL_STATUS_VALUE EQUAL 0)
            list(GET FETK_DL_STATUS 1 FETK_DL_STATUS_STRING)
            message(FATAL_ERROR "FETK download error: ${FETK_DL_STATUS_STRING}")
        endif()
 
        # unzip
        message(STATUS "Unzipping: ${FETK_UNZIP_COMMAND} ${FETK_UNZIP_OPTS} ${FETK_PATH}/${FETK_ZIP_FILE_NAME}")
        execute_process(
            COMMAND ${FETK_UNZIP_COMMAND} ${FETK_UNZIP_OPTS} ${FETK_PATH}/${FETK_ZIP_FILE_NAME}
            COMMAND_ECHO STDERR
            WORKING_DIRECTORY ${FETK_PATH}
            RESULT_VARIABLE FETK_UNZIP_RETURN
        )
        if(NOT FETK_UNZIP_RETURN EQUAL 0)
            message(FATAL_ERROR "Unzip operation failed")
        endif()
        
        file(MAKE_DIRECTORY ${FETK_BUILD_PATH})
        message(STATUS "Building in ${FETK_BUILD_PATH}")

        if(WIN32)
            set(FETK_CONFIG_COMMAND, "cmake -DCMAKE_BUILD_TYPE=Release -DCREATE_PACKAGE=FALSE -DVCPKG_TARGET_TRIPLET=x86-windows -DCMAKE_TOOLCHAIN_FILE=${VCPKG_INSTALLATION_ROOT}/scripts/buildsystems/vcpkg.cmake -G \"Visual Studio 16 2019\" -A Win32 ..")
            set(FETK_BUILD_COMMAND, "cmake --build . --config Release --parallel 1")
        else()
            set(FETK_CONFIG_COMMAND, "cmake -D CMAKE_INSTALL_PREFIX=${FETK_BUILD_PATH} -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ..")
            set(FETK_BUILD_COMMAND, "make -j install")
    endif()

        # cmake configuration of FETK; occurs at configure time
        execute_process( 
            COMMAND ${FETK_CONFIG_COMMAND}
            WORKING_DIRECTORY ${FETK_BUILD_PATH}
            RESULT_VARIABLE FETK_BUILD_RETURN
        )
        if(NOT FETK_BUILD_RETURN EQUAL 0)
            message(FATAL_ERROR "FETK build operation failed")
        endif()
        
        execute_process( 
            COMMAND ${FETK_BUILD_COMMAND}
            WORKING_DIRECTORY ${FETK_BUILD_PATH}
            RESULT_VARIABLE FETK_BUILD_RETURN
        )
        if(NOT FETK_BUILD_RETURN EQUAL 0)
            message(FATAL_ERROR "FETK build operation failed")
        endif()

        # Second we use the ExternalProject module to build and install FETK.
        # This takes place during the build phase of APBS
        # Note that we disable the configure stage of the ExternalProject because we already took care of the configure stage above.
        #include(ExternalProject)
        
        #ExternalProject_Add( FETK
        #    PREFIX            ${FETK_BUILD_PATH}
        #    SOURCE_DIR        ${FETK_PATH}/${FETK_NAME}
        #    BINARY_DIR        ${FETK_BUILD_PATH}
        #    CONFIGURE_COMMAND ""
        #    BUILD_ALWAYS      1
        #    BUILD_COMMAND     $(MAKE)
        #    INSTALL_DIR       ${FETK_BUILD_PATH}
        #    INSTALL_COMMAND   $(MAKE) install
        #)
    endif(FETK_FROM_PKG)

    include_directories(${FETK_INSTALL_PREFIX}/include)
    link_directories(${FETK_INSTALL_PREFIX}/lib)

    if(WIN32)
        list(APPEND APBS_LIBS
            maloc
            umfpack
            blas
        )
    else()
        list(APPEND APBS_LIBS
            maloc
#           amd
            punc
            mc
            gamer
#           superlu
            umfpack
            blas
            vf2c
#           tetgen
#           triangle
#           readline
#           termcap
        )
    endif()

    SET(HAVE_MC_H 1)
    SET(HAVE_PUNC_H 1)
    SET(HAVE_BIOM_H 1)

endif(ENABLE_FETK)

