Python3_add_library(Figure4 MODULE Figure4.cpp)
target_compile_features(Figure4 PRIVATE cxx_std_23)
set_target_properties(Figure4 PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_link_libraries(Figure4 PRIVATE asio::asio)
