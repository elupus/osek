add_executable(Os_HCS12 Os_Cfg.c)
target_include_directories(Os_HCS12 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Os_HCS12 PRIVATE OsekOs::OsekOs)

if(CMAKE_OBJCOPY AND CMAKE_OBJDUMP)
    add_custom_command(TARGET Os_HCS12 POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --output-format=srec $<TARGET_FILE:Os_HCS12> $<TARGET_FILE_DIR:Os_HCS12>/Os_HCS12.s19
        COMMAND ${CMAKE_OBJDUMP} $<TARGET_FILE:Os_HCS12> -S --disassemble > $<TARGET_FILE_DIR:Os_HCS12>/Os_HCS12.lst
        BYPRODUCTS Os_HCS12.s19 Os_HCS12.lst
        VERBATIM
    )
endif()

if(CHC12_BURNER)
    add_custom_command(TARGET Os_HCS12 POST_BUILD
        COMMAND ${CHC12_PIPER} ${CHC12_BURNER} -ViewHidden -EnvABS_FILE=$<TARGET_FILE:Os_HCS12> -F ${PROJECT_SOURCE_DIR}/test/common/hcs12/burner.bbl
        VERBATIM
    )
endif()
