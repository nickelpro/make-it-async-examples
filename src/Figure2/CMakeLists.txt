Python3_add_library(Figure2 MODULE Figure2.cpp)
target_compile_features(Figure2 PRIVATE cxx_std_23)
set_target_properties(Figure2 PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_link_libraries(Figure2 PRIVATE asio::asio)
