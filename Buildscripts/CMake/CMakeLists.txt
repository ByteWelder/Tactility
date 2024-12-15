idf_component_register(
    INCLUDE_DIRS "Libraries/TactilityC/Include" "Libraries/lvgl/Include"
)

add_prebuilt_library(TactilityC Libraries/TactilityC/Binary/libTactilityC.a)
add_prebuilt_library(lvgl Libraries/lvgl/Binary/liblvgl.a)

target_link_libraries(${COMPONENT_LIB} INTERFACE TactilityC)
target_link_libraries(${COMPONENT_LIB} INTERFACE lvgl)
