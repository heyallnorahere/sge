cmake_minimum_required(VERSION 3.10)

managed_target(example-scripts "${CMAKE_CURRENT_SOURCE_DIR}/example-scripts.csproj")
set_target_properties(example-scripts PROPERTIES FOLDER "examples")
add_dependencies(sge example-scripts)