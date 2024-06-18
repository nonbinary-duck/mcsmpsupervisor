# Add main.cpp to the executable
add_executable(<EXEC> main.cpp)

# Use c++ 23 if supported
set_property(TARGET <EXEC> PROPERTY CXX_STANDARD 23)

# Add other sources from this dir
target_sources(<EXEC> PRIVATE
    utils.hpp
)