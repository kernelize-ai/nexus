project(nexus-cpu-test)

add_library(cpu_kernel SHARED
  cpu_kernel.c
  )
set_target_properties(cpu_kernel PROPERTIES PREFIX "" SUFFIX ".so")


add_executable(nexus-cpu-test 
  main.cpp )
target_link_libraries(nexus-cpu-test PRIVATE nexus-api)

