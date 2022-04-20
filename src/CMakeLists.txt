add_executable(shades gl.c glad.c shades.c)
target_link_libraries(shades PRIVATE m glfw)
target_compile_options(shades PUBLIC -Wall -Wextra -Werror)
