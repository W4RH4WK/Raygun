function(raygun_add_shader target shader dependencies)
    find_program(GLSLC glslc)
    if(NOT GLSLC)
        message(FATAL_ERROR "glslc not found")
    endif()

    add_custom_command(
        OUTPUT ${shader}.spv
        COMMAND ${GLSLC} -o ${shader}.spv ${shader}
        MAIN_DEPENDENCY ${shader}
        DEPENDS ${dependencies}
    )

    set_source_files_properties(${shader}.spv PROPERTIES GENERATED TRUE)
    target_sources(${target} PRIVATE ${shader})
endfunction()
