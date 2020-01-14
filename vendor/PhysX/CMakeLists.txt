if(MSVC)
    add_library(PhysX SHARED IMPORTED GLOBAL)
    target_include_directories(PhysX INTERFACE include)
    set_target_properties(PhysX PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysX_64.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysX_64.dll
    )

    add_library(PhysX_Common SHARED IMPORTED)
    set_target_properties(PhysX_Common PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCommon_64.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCommon_64.dll
    )

    add_library(PhysX_Cooking SHARED IMPORTED)
    set_target_properties(PhysX_Cooking PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCooking_64.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCooking_64.dll
    )

    add_library(PhysX_Extensions STATIC IMPORTED)
    set_target_properties(PhysX_Extensions PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXExtensions_static_64.lib
    )

    add_library(PhysX_Foundation SHARED IMPORTED)
    set_target_properties(PhysX_Foundation PROPERTIES
        IMPORTED_IMPLIB   ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXFoundation_64.lib
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXFoundation_64.dll
    )

    add_library(PhysX_PvdSDK STATIC IMPORTED)
    set_target_properties(PhysX_PvdSDK PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXPvdSDK_static_64.lib
    )

    target_link_libraries(PhysX INTERFACE
        PhysX_Common
        PhysX_Cooking
        PhysX_Extensions
        PhysX_Foundation
        PhysX_PvdSDK
    )

    raygun_copy_dlls(
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysX_64.dll
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCommon_64.dll
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXCooking_64.dll
        ${CMAKE_CURRENT_SOURCE_DIR}/bin/$(Configuration)/PhysXFoundation_64.dll
    )
else()
    if(NOT PhysX_ROOT)
        message(WARNING "PhysX_ROOT not set")
    endif()

    add_library(PhysX STATIC IMPORTED GLOBAL)
    target_include_directories(PhysX INTERFACE
        ${PhysX_ROOT}/physx/include
        ${PhysX_ROOT}/pxshared/include
    )
    set_target_properties(PhysX PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysX_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysX_static_64.a
    )

    add_library(PhysX_Common STATIC IMPORTED)
    set_target_properties(PhysX_Common PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXCommon_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXCommon_static_64.a
    )

    add_library(PhysX_Cooking STATIC IMPORTED)
    set_target_properties(PhysX_Cooking PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXCooking_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXCooking_static_64.a
    )

    add_library(PhysX_Extensions STATIC IMPORTED)
    set_target_properties(PhysX_Extensions PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXExtensions_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXExtensions_static_64.a
    )

    add_library(PhysX_Foundation STATIC IMPORTED)
    set_target_properties(PhysX_Foundation PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXFoundation_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXFoundation_static_64.a
    )

    add_library(PhysX_PvdSDK STATIC IMPORTED)
    set_target_properties(PhysX_PvdSDK PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXPvdSDK_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXPvdSDK_static_64.a
    )

    add_library(PhysX_Vehicle STATIC IMPORTED)
    set_target_properties(PhysX_Vehicle PROPERTIES
        IMPORTED_LOCATION_DEBUG   ${PhysX_ROOT}/physx/bin/linux.clang/debug/libPhysXVehicle_static_64.a
        IMPORTED_LOCATION_RELEASE ${PhysX_ROOT}/physx/bin/linux.clang/release/libPhysXVehicle_static_64.a
    )

    target_link_libraries(PhysX INTERFACE
        PhysX_Cooking
        PhysX_Common
        PhysX_Extensions
        PhysX_PvdSDK
        PhysX_Foundation
        PhysX_Vehicle
    )
endif()
