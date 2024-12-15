get_filename_component(VERSION_TEXT_FILE ../version.txt ABSOLUTE)

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

message("\n\n\
                      ${LightPurple}@@\n\
                     @@@\n\
                     @@@\n\
                     @@@\n\
                     @@@\n\
     ${Cyan}@@@@@@@@@@@@@@@@${LightPurple}@@@\n\
    ${Cyan}@@@@@@@@@@@@@@@@@${LightPurple}@@@\n\
              ${Cyan}@@@    ${LightPurple}@@@                 ${White}Tactility ${TACTILITY_VERSION}\n\
              ${Cyan}@@@    ${LightPurple}@@@                 ${Grey}${TACTILITY_TARGET}\n\
              ${Cyan}@@@${LightPurple}@@@@@@@@@@@@@@@@@\n\
              ${Cyan}@@@${LightPurple}@@@@@@@@@@@@@@@@\n\
              ${Cyan}@@@\n\
              @@@\n\
              @@@\n\
              @@@\n\
              @@\n\n")
