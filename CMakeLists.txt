cmake_minimum_required(VERSION 3.14)
# Initialize the "profinet" project (this is the name the test file looks for!)
project(profinet C CXX)

# --------------------------------------------------------------------
# 1. Compiler settings and General Options
# --------------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_TESTING "Build unit tests" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# --------------------------------------------------------------------
# 2. Profinet Stack Options (Default to 0, configurable via cmake)
# --------------------------------------------------------------------
set(PNET_OPTION_FAST_STARTUP 0 CACHE STRING "Fast startup")
set(PNET_OPTION_PARAMETER_SERVER 0 CACHE STRING "Parameter server")
set(PNET_OPTION_IR 0 CACHE STRING "")
set(PNET_OPTION_SR 0 CACHE STRING "")
set(PNET_OPTION_REDUNDANCY 0 CACHE STRING "")
set(PNET_OPTION_AR_VENDOR_BLOCKS 0 CACHE STRING "")
set(PNET_OPTION_RS 0 CACHE STRING "")
set(PNET_OPTION_MC_CR 0 CACHE STRING "")
set(PNET_OPTION_SRL 0 CACHE STRING "")
set(PNET_OPTION_SNMP 0 CACHE STRING "")
set(PNET_OPTION_DRIVER_ENABLE 0 CACHE STRING "")
set(PNET_USE_ATOMICS 0 CACHE STRING "")

# Memory (Default numbers)
set(PNET_MAX_AR 2)
set(PNET_MAX_API 1)
set(PNET_MAX_CR 2)
set(PNET_MAX_SLOTS 16)
set(PNET_MAX_SUBSLOTS 16)
set(PNET_MAX_DFP_IOCR 0)
set(PNET_MAX_PHYSICAL_PORTS 1)
set(PNET_MAX_LOG_BOOK_ENTRIES 32)
set(PNET_MAX_ALARMS 10)
set(PNET_MAX_ALARM_PAYLOAD_DATA_SIZE 256)
set(PNET_MAX_DIAG_ITEMS 100)
set(PNET_MAX_DIAG_MANUF_DATA_SIZE 64)
set(PNET_MAX_MC_CR 0)
set(PNET_MAX_AR_VENDOR_BLOCKS 0)
set(PNET_MAX_AR_VENDOR_BLOCK_DATA_LENGTH 0)
set(PNET_MAX_MAN_SPECIFIC_FAST_STARTUP_DATA_LENGTH 512)
set(PNET_MAX_SESSION_BUFFER_SIZE 256)
set(PNET_MAX_DIRECTORYPATH_SIZE 256)
set(PNET_MAX_FILENAME_SIZE 256)
set(PNET_MAX_PORT_DESCRIPTION_SIZE 256)

# Logging Variables
set(LOG_LEVEL ERR CACHE STRING "")
set(PF_ETH_LOG OFF CACHE STRING "")
set(PF_LLDP_LOG OFF CACHE STRING "")
set(PF_SNMP_LOG OFF CACHE STRING "")
set(PF_CPM_LOG OFF CACHE STRING "")
set(PF_PPM_LOG OFF CACHE STRING "")
set(PF_DCP_LOG OFF CACHE STRING "")
set(PF_RPC_LOG OFF CACHE STRING "")
set(PF_ALARM_LOG OFF CACHE STRING "")
set(PF_AL_BUF_LOG OFF CACHE STRING "")
set(PF_PNAL_LOG OFF CACHE STRING "")
set(PNET_LOG OFF CACHE STRING "")

# Dynamic header generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pnet_options.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/pnet_options.h
    @ONLY
)

set(PNET_VERSION_MAJOR 1)
set(PNET_VERSION_MINOR 0)
set(PNET_VERSION_PATCH 0)
set(PNET_VERSION_HASH "459e043")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pnet_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/pnet_version.h
    @ONLY
)

# --------------------------------------------------------------------
# 3. Automatic Dependency Management: cmake-tools and OSAL
# --------------------------------------------------------------------
include(FetchContent)

# Download cmake-tools automatically
FetchContent_Declare(
    cmake_tools
    GIT_REPOSITORY https://github.com/rtlabs-com/cmake-tools.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(cmake_tools)
list(APPEND CMAKE_MODULE_PATH ${cmake_tools_SOURCE_DIR})

# Try to include the AddOsal macro mentioned in the guide
include(AddOsal OPTIONAL RESULT_VARIABLE ADDOSAL_FOUND)

# If the macro or OSAL is not found, download it automatically (Case 1 from the documentation)
if(NOT ADDOSAL_FOUND AND NOT TARGET osal)
    message(STATUS "Downloading OSAL automatically...")
    FetchContent_Declare(
        osal
        GIT_REPOSITORY https://github.com/rtlabs-com/osal.git
        GIT_TAG        master
    )
    FetchContent_MakeAvailable(osal)
endif()

# --------------------------------------------------------------------
# 4. Profinet Library Creation
# --------------------------------------------------------------------
file(GLOB_RECURSE PNET_SOURCES 
    "src/common/*.c" 
    "src/device/*.c"
)

add_library(profinet STATIC ${PNET_SOURCES})

target_include_directories(profinet PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

# Link OSAL to Profinet
if(TARGET osal)
    target_link_libraries(profinet PUBLIC osal)
endif()

# --------------------------------------------------------------------
# 5. Install Target (To export scripts and binaries, as per README)
# --------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS profinet
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install scripts from the 'tools' folder (e.g., IP settings, control LEDs)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/ DESTINATION ${CMAKE_INSTALL_BINDIR} USE_SOURCE_PERMISSIONS OPTIONAL)

# --------------------------------------------------------------------
# 6. Unit Tests
# --------------------------------------------------------------------
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt")
    enable_testing()
    add_subdirectory(test)
endif()
