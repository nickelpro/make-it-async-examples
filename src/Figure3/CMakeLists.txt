Python3_add_library(Figure3 MODULE Figure3.cpp)
target_compile_features(Figure3 PRIVATE cxx_std_23)
set_target_properties(Figure3 PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_link_libraries(Figure3 PRIVATE asio::asio)
