get_filename_component(VERSION_TEXT_FILE version.txt ABSOLUTE)

file(READ ${VERSION_TEXT_FILE} TACTILITY_VERSION)

if (DEFINED ENV{ESP_IDF_VERSION})
    set(TACTILITY_TARGET "   @ ESP-IDF")
else()
    set(TACTILITY_TARGET "  @ Simulator")
endif()

if(NOT WIN32)
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(Cyan        "${Esc}[36m")
    set(Grey        "${Esc}[37m")
    set(LightPurple "${Esc}[1;35m")
    set(White       "${Esc}[1;37m")
else()
    set(ColourReset "")
    set(Cyan        "")
    set(Grey        "")
    set(LightPurple "")
    set(White       "")
endif()

# Some terminals (e.g. GitHub Actions) reset colour for every in a multiline message(),
# so we add the colour to each line instead of assuming it would automatically be re-used.
message("\n\n\
                      ${LightPurple}@@\n\
                     ${LightPurple}@@@\n\
                     ${LightPurple}@@@\n\
                     ${LightPurple}@@@\n\
                     ${LightPurple}@@@\n\
     ${Cyan}@@@@@@@@@@@@@@@@${LightPurple}@@@\n\
    ${Cyan}@@@@@@@@@@@@@@@@@${LightPurple}@@@\n\
              ${Cyan}@@@    ${LightPurple}@@@                 ${White}Tactility ${TACTILITY_VERSION}\n\
              ${Cyan}@@@    ${LightPurple}@@@                 ${Grey}${TACTILITY_TARGET}\n\
              ${Cyan}@@@${LightPurple}@@@@@@@@@@@@@@@@@\n\
              ${Cyan}@@@${LightPurple}@@@@@@@@@@@@@@@@\n\
              ${Cyan}@@@\n\
              ${Cyan}@@@\n\
              ${Cyan}@@@\n\
              ${Cyan}@@@\n\
              ${Cyan}@@\n\n${ColourReset}")
