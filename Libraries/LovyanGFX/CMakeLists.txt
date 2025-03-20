set(LGFX_ROOT ${CMAKE_CURRENT_LIST_DIR})

# Prevent multiple inclusions
if (NOT TARGET __idf_LovyanGFX)
    include(${LGFX_ROOT}/boards.cmake/esp-idf.cmake)
endif()
