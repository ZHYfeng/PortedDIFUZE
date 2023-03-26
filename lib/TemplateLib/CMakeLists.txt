set(SOURCES
        define.h)

add_library(TemplateLib ${SOURCES})
set_target_properties(TemplateLib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(TemplateLib PUBLIC
        ToolLib
        )