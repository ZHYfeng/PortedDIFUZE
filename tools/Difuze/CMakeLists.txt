add_executable(Difuze Difuze.cpp)
install(TARGETS Difuze DESTINATION bin)
target_link_libraries(Difuze PUBLIC
        ManagerLib
        #        PkgConfig::YOUR_PKG
        )