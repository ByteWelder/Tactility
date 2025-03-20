# Prevent multiple inclusions
if (TARGET __idf_LovyanGFX)
    return()
endif()

# Define component name
set(COMPONENT_NAME LovyanGFX)

# Ensure LGFX_ROOT is properly defined
if (NOT DEFINED LGFX_ROOT)
    set(LGFX_ROOT ${CMAKE_CURRENT_LIST_DIR})
endif()

# Include directories
set(COMPONENT_ADD_INCLUDEDIRS
    ${LGFX_ROOT}/src
)

# Collect all source files
file(GLOB SRCS
    ${LGFX_ROOT}/src/lgfx/Fonts/efont/*.c
    ${LGFX_ROOT}/src/lgfx/Fonts/IPA/*.c
    ${LGFX_ROOT}/src/lgfx/utility/*.c
    ${LGFX_ROOT}/src/lgfx/v1/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/misc/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/panel/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/platforms/arduino_default/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/platforms/esp32/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/platforms/esp32c3/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/platforms/esp32s2/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/platforms/esp32s3/*.cpp
    ${LGFX_ROOT}/src/lgfx/v1/touch/*.cpp
)

# Define required components based on ESP-IDF version
if (IDF_VERSION_MAJOR GREATER_EQUAL 5)
    set(COMPONENT_REQUIRES nvs_flash efuse esp_lcd driver esp_timer)
elseif ((IDF_VERSION_MAJOR EQUAL 4) AND (IDF_VERSION_MINOR GREATER 3) OR IDF_VERSION_MAJOR GREATER 4)
    set(COMPONENT_REQUIRES nvs_flash efuse esp_lcd)
else()
    set(COMPONENT_REQUIRES nvs_flash efuse)
endif()

# Remove self-dependency if it accidentally appears in the list
list(REMOVE_ITEM COMPONENT_REQUIRES LovyanGFX)

# Register the component
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${LGFX_ROOT}/src
    REQUIRES ${COMPONENT_REQUIRES}
)

message(STATUS "Registered LovyanGFX with components: ${COMPONENT_REQUIRES}")
