cmake_minimum_required(VERSION 3.10)

managed_target(scriptcore "${CMAKE_CURRENT_SOURCE_DIR}/scriptcore.csproj")
set_target_properties(scriptcore PROPERTIES FOLDER "core")