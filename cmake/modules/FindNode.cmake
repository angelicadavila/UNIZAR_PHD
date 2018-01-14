function(FindNode)
  if (NOT NODE)
    set (NODE $ENV{NODE})
    if (NOT NODE)
      execute_process(COMMAND "hostname"
        OUTPUT_VARIABLE NODE
        )
    endif()
    set (NODE "${NODE}" PARENT_SCOPE)
  endif()
endfunction()
