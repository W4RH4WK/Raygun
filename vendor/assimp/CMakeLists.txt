if(MSVC)
    add_library(assimp SHARED IMPORTED GLOBAL)
    target_include_directories(assimp INTERFACE include)
    set_target_properties(assimp PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/assimp-vc140-mt.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/assimp-vc140-mt.dll
    )
    raygun_copy_dlls(${CMAKE_CURRENT_SOURCE_DIR}/bin/assimp-vc140-mt.dll)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Assimp REQUIRED IMPORTED_TARGET GLOBAL assimp)
    add_library(assimp ALIAS PkgConfig::Assimp)
endif()

