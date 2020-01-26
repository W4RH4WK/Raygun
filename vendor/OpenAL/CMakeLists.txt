if(MSVC)
    add_library(OpenAL SHARED IMPORTED GLOBAL)
    target_include_directories(OpenAL INTERFACE include)
    set_target_properties(OpenAL PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/OpenAL32.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/OpenAL32.dll
    )
    raygun_copy_dlls(${CMAKE_CURRENT_SOURCE_DIR}/bin/OpenAL32.dll)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(OpenAL REQUIRED IMPORTED_TARGET GLOBAL openal)
    add_library(OpenAL ALIAS PkgConfig::OpenAL)
endif()
