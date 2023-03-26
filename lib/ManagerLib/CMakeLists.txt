set(SOURCES
        Manager.cpp Manager.h)

add_library(ManagerLib ${SOURCES})
set_target_properties(ManagerLib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(ManagerLib PUBLIC
        AnalysisLib
        ToolLib
        DIFUZE
        DIFUZE_FIX
        stdc++fs
        )