CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(typed_matrix)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)

FIND_PACKAGE(xlsx_reader CONFIG REQUIRED)
FIND_PACKAGE(any_container CONFIG REQUIRED)
FIND_PACKAGE(typed_string CONFIG REQUIRED)
FIND_PACKAGE(miniz CONFIG REQUIRED)
FIND_PACKAGE(tinyxml2 CONFIG REQUIRED)
FIND_PACKAGE(nlohmann_json CONFIG REQUIRED)

INCLUDE_DIRECTORIES(${typed_string_include_dirs})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

FILE(GLOB_RECURSE typed_matrix_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/base/*.cpp)
FILE(GLOB_RECURSE typed_matrix_EXPORTER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/exporter/*.cpp)

add_library(typed_matrix ${typed_matrix_SRCS})

add_library(typed_matrix_exporter ${typed_matrix_EXPORTER_SRCS})
target_link_libraries(typed_matrix PUBLIC typed_string)
target_include_directories(typed_matrix PUBLIC ${typed_string_include_dirs})
target_link_libraries(typed_matrix_exporter PUBLIC typed_matrix  xlsx_reader)

add_subdirectory(test)

foreach(p LIB BIN INCLUDE)
	set(var CMAKE_INSTALL_${p}DIR)
	if(NOT IS_ABSOLUTE "${${var}}")
		string(TOLOWER ${p} lower_p)
        set(${var} "${CMAKE_INSTALL_PREFIX}/${lower_p}")
	endif()
endforeach()

export(TARGETS typed_matrix  typed_matrix_exporter
     FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Targets.cmake)

file(WRITE
    ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    "include(\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Targets.cmake)\n")

install(FILES
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(TARGETS typed_matrix  typed_matrix_exporter
 EXPORT ${PROJECT_NAME}Targets
   ARCHIVE  DESTINATION lib
   LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
  )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME})

install(EXPORT ${PROJECT_NAME}Targets
		NAMESPACE typed_matrix::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})