add_executable(Figure1 Figure1.cpp)
target_compile_features(Figure1 PRIVATE cxx_std_23)
target_link_libraries(Figure1 PRIVATE asio::asio)
