# Non-standalone test file, generated from the original Makefile.am file

set (BIN_NAME exrmaketiled)
set (SRCS
    main.cpp Image.h Image.cpp makeTiled.cpp makeTiled.h
)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

add_executable (${BIN_NAME} ${SRCS})
target_link_libraries (${BIN_NAME} IlmImf ${ZLIB_LIBRARIES})

install(TARGETS ${BIN_NAME}
        RUNTIME DESTINATION ${INSTALL_RUNTIME_DIR})
