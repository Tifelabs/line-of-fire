add_executable(lof_tests
    test_main.cpp
    test_fixed.cpp
    test_intersect.cpp
    test_sim_determinism.cpp
    test_stroke.cpp)
target_link_libraries(lof_tests PRIVATE lof_render_core doctest::doctest)
add_test(NAME lof_tests COMMAND lof_tests)
