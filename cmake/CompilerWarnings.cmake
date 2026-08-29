function(femcae_enable_warnings target_name)
    if(CMAKE_Fortran_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            $<$<COMPILE_LANGUAGE:Fortran>:-Wall>
            $<$<COMPILE_LANGUAGE:Fortran>:-Wextra>
            $<$<COMPILE_LANGUAGE:Fortran>:-Wimplicit-interface>
            $<$<COMPILE_LANGUAGE:Fortran>:-Werror=implicit-interface>
            $<$<COMPILE_LANGUAGE:Fortran>:-Wconversion-extra>
            $<$<COMPILE_LANGUAGE:Fortran>:-Wcharacter-truncation>
        )

        if(FEMCAE_ENABLE_RUNTIME_CHECKS)
            target_compile_options(${target_name} PRIVATE
                $<$<AND:$<COMPILE_LANGUAGE:Fortran>,$<CONFIG:Debug>>:-fcheck=all>
                $<$<AND:$<COMPILE_LANGUAGE:Fortran>,$<CONFIG:Debug>>:-fbacktrace>
            )
        endif()
    elseif(CMAKE_Fortran_COMPILER_ID MATCHES "IntelLLVM")
        target_compile_options(${target_name} PRIVATE
            $<$<COMPILE_LANGUAGE:Fortran>:-warn>
            $<$<COMPILE_LANGUAGE:Fortran>:all>
        )
    endif()
endfunction()
