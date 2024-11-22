idf_component_register(
    SRC_DIRS "source"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private"
    REQUIRES Tactility m5stack_shared vfs fatfs M5Unified
)
