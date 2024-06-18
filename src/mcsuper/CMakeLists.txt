# Add main.cpp to the executable
add_executable(mcsuper main.cpp)

# Use c++ 23 if supported
set_property(TARGET mcsuper PROPERTY CXX_STANDARD 23)

# Add other sources from this dir
target_sources(mcsuper PRIVATE
    utils.hpp
)