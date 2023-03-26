set(SOURCES
        T_module.cpp T_module.h
        )

add_library(AnalysisLib ${SOURCES})
set_target_properties(AnalysisLib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(AnalysisLib PUBLIC
        ToolLib
        )