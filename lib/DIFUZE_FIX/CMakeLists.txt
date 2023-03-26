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

add_library(DIFUZE_FIX ${SOURCES})
set_target_properties(DIFUZE_FIX PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(DIFUZE_FIX PUBLIC
        ToolLib
        TemplateLib
        )