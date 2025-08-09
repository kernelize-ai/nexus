# Google Test
include_directories(../third_party/googletest)

enable_testing()
include(GoogleTest)

function(add_nexus_it)
  set(options)
  set(oneValueArgs NAME)
  set(multiValueArgs SRCS LIBS DEFS)
  cmake_parse_arguments(_ "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_property(nexus_libs GLOBAL PROPERTY NEXUS_LIBRARIES)

  add_test(NAME ${__NAME}
          COMMAND ${__NAME})
  add_executable(
          ${__NAME}
          ${__SRCS})
  target_link_libraries(
          ${__NAME}
          PRIVATE
          GTest::gtest_main
          ${nexus_libs}
          gmock
          ${__LIBS})

  if(NOT MSVC)
    target_compile_options(${__NAME} PRIVATE -fno-rtti)
  endif()

  target_compile_definitions(${__NAME} PRIVATE ${__DEFS})

  # Without the TEST_DISCOVERY_TIMEOUT, the tests randomly time out on my mac
  # laptop.  I think the issue may be that the very first time you run a program
  # it's a bit slow.
  gtest_discover_tests(${__NAME} DISCOVERY_TIMEOUT 60)

  # Add the unit test to the top-level unit test target.
  # add_dependencies(NexusIntegrationTest ${__NAME})
endfunction()

add_subdirectory(kernels)
add_subdirectory(cpp)
#add_subdirectory(pynexus)
