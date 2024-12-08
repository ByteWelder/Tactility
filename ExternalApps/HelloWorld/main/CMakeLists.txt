file(GLOB_RECURSE SOURCE_FILES Source/*.c)

idf_component_register(
    SRCS ${SOURCE_FILES}
)

add_prebuilt_library(Tactility ../../../build/esp-idf/Tactility/libTactility.a)
add_prebuilt_library(TactilityC ../../../build/esp-idf/TactilityC/libTactilityC.a)
add_prebuilt_library(TactilityCore ../../../build/esp-idf/TactilityCore/libTactilityCore.a)
add_prebuilt_library(TactilityHeadless ../../../build/esp-idf/TactilityHeadless/libTactilityHeadless.a)
add_prebuilt_library(lvgl ../../../build/esp-idf/lvgl/liblvgl.a)

include_directories("../../../TactilityC/Source")
include_directories("../../../Libraries/lvgl/src")

target_link_libraries(${COMPONENT_LIB} PRIVATE TactilityC Tactility TactilityCore TactilityHeadless lvgl)
