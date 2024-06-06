add_executable(AppendixA AppendixA.cpp)
target_compile_features(AppendixA PRIVATE cxx_std_23)
target_link_libraries(AppendixA PRIVATE asio::asio)
