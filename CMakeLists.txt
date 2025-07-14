# Require CMake version 3.10 or higher
cmake_minimum_required(VERSION 3.10)

# Name your project
project(RealtimeSensorAggregator)

# Add an executable target for the listener
add_executable(listener listener.cpp)

# Add an executable target for the aggregator
add_executable(aggregator aggregator.cpp)