if(MSVC)
    add_library(ogg STATIC IMPORTED)
    set_target_properties(ogg PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/libogg_static.lib)

    add_library(opusfile STATIC IMPORTED)
    set_target_properties(opusfile PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/opusfile.lib)

    add_library(opus STATIC IMPORTED GLOBAL)
    set_target_properties(opus PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/opus.lib)

    target_include_directories(opus INTERFACE include include/opus)

    target_link_libraries(opus INTERFACE ogg opusfile)
else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(Ogg REQUIRED IMPORTED_TARGET ogg)
    pkg_check_modules(Opus REQUIRED IMPORTED_TARGET opus)
    pkg_check_modules(Opusfile REQUIRED IMPORTED_TARGET opusfile)

    add_library(opus INTERFACE)
    target_link_libraries(opus INTERFACE
        PkgConfig::Ogg
        PkgConfig::Opus
        PkgConfig::Opusfile
    )
endif()

