function(debug_vars)
  get_cmake_property(_variableNames VARIABLES)
  list (SORT _variableNames)
  foreach (_variableName ${_variableNames})
      message(STATUS "${_variableName}=${${_variableName}}")
  endforeach()
endfunction()

function(bon_add_executable EXE_NAME)
  file(GLOB EXE_SOURCES
      "*.hpp"
      "*.cpp"
  )
  add_executable(${EXE_NAME} ${EXE_SOURCES})
  set_target_properties(${EXE_NAME} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()


function(bon_add_library LIB_NAME)
  file(GLOB LIB_SOURCES
      "*.hpp"
      "*.cpp"
  )
  add_library(${LIB_NAME} ${LIB_SOURCES})
  set_target_properties(${LIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()
