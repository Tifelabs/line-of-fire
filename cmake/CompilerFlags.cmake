# One interface target carries every flag the project cares about.
add_library(lof_flags INTERFACE)

if(MSVC)
    target_compile_options(lof_flags INTERFACE /W4 /fp:precise)
else()
    # -ffp-contract=off: no fused multiply-add surprises across CPUs.
    # Never use -ffast-math in this project.
    target_compile_options(lof_flags INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -ffp-contract=off)
    if(LOF_SANITIZE)
        target_compile_options(lof_flags INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(lof_flags INTERFACE -fsanitize=address,undefined)
    endif()
endif()
