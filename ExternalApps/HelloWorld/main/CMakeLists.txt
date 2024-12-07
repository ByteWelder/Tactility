file(GLOB_RECURSE SOURCE_FILES *.c)

idf_component_register(
    SRCS main.c #${SOURCE_FILES}
)
