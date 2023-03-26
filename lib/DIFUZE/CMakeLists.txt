set(SOURCES
        CFGUtils.h
        CFGUtils.cpp

        DeviceNameFinder.cpp
        EntryPointIdentifier.cpp

        TypePrintHelper.h
        TypePrintHelper.cpp
        IOInstVisitor.h
        IOInstVisitor.cpp
        NewIoctlCmdParser.cpp)

add_library(DIFUZE ${SOURCES})
set_target_properties(DIFUZE PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(DIFUZE PUBLIC
        ToolLib
        TemplateLib
        )