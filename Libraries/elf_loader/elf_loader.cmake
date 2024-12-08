# The script is to generate ELF for application

# Trick to temporarily redefine project(). When functions are overriden in CMake, the originals can still be accessed
# using an underscore prefixed function of the same name. The following lines make sure that project  calls
# the original project(). See https://cmake.org/pipermail/cmake/2015-October/061751.html.
function(project_elf)
endfunction()

function(_project_elf)
endfunction()

macro(project_elf project_name)
    # Enable these options to remove unused symbols and reduce linked objects
    set(cflags -nostartfiles
               -nostdlib
               -fPIC
               -shared
               -e app_main
               -fdata-sections
               -ffunction-sections
               -Wl,--gc-sections
               -fvisibility=hidden)

    # Enable this options to remove unnecessary sections in
    list(APPEND cflags -Wl,--strip-all
                       -Wl,--strip-debug
                       -Wl,--strip-discarded)

    list(APPEND cflags -Dmain=app_main)

    idf_build_set_property(COMPILE_OPTIONS "${cflags}" APPEND)

    set(elf_app "${CMAKE_PROJECT_NAME}.app.elf")

    # Remove more unused sections
    string(REPLACE "-elf-gcc" "-elf-strip" ${CMAKE_STRIP} ${CMAKE_C_COMPILER})
    set(strip_flags --strip-unneeded
                    --remove-section=.xt.lit
                    --remove-section=.xt.prop
                    --remove-section=.comment
                    --remove-section=.xtensa.info
                    --remove-section=.got.loc
                    --remove-section=.got)

    # Link input list of libraries to ELF
    if(ELF_COMPONENTS)
        foreach(c "${ELF_COMPONENTS}")
            set(elf_libs "${elf_libs}" "esp-idf/${c}/lib${c}.a")
        endforeach()
    endif()

    set(elf_libs ${elf_libs} "${ELF_LIBS}" "esp-idf/main/libmain.a")
    spaces2list(elf_libs)

    add_custom_command(OUTPUT elf_app
        COMMAND ${CMAKE_C_COMPILER} ${cflags} ${elf_libs} -o ${elf_app}
        COMMAND ${CMAKE_STRIP} ${strip_flags} ${elf_app}
        DEPENDS idf::main
        COMMENT "Build ELF: ${elf_app}"
        )
    add_custom_target(elf ALL DEPENDS elf_app)
endmacro()
