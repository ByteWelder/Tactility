idf_component_register(
    INCLUDE_DIRS "Libraries/Tactility/Include" "Libraries/lvgl/Include"
)

add_prebuilt_library(Tactility Libraries/Tactility/Binary/libTactility.a)
add_prebuilt_library(TactilityC Libraries/Tactility/Binary/libTactilityC.a)
add_prebuilt_library(TactilityCore Libraries/Tactility/Binary/libTactilityCore.a)
add_prebuilt_library(TactilityHeadless Libraries/Tactility/Binary/libTactilityHeadless.a)
add_prebuilt_library(elf_loader Libraries/elf_loader/Binary/libelf_loader.a)
add_prebuilt_library(lvgl Libraries/lvgl/Binary/liblvgl.a)

target_link_libraries(${COMPONENT_LIB} INTERFACE TactilityC)
target_link_libraries(${COMPONENT_LIB} INTERFACE Tactility)
target_link_libraries(${COMPONENT_LIB} INTERFACE TactilityHeadless)
target_link_libraries(${COMPONENT_LIB} INTERFACE TactilityCore)
target_link_libraries(${COMPONENT_LIB} INTERFACE lvgl)
target_link_libraries(${COMPONENT_LIB} INTERFACE elf_loader)
