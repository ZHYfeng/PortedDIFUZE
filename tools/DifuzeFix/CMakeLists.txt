add_executable(DifuzeFix DifuzeFix.cpp)
install(TARGETS DifuzeFix DESTINATION bin)
target_link_libraries(DifuzeFix PUBLIC
        ManagerLib
        #        PkgConfig::YOUR_PKG
        )