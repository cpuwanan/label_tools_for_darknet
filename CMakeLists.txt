cmake_minimum_required(VERSION 2.6)
project(cpp_helpers_for_deep_learning)

add_definitions(-std=c++11)

find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread signals filesystem)

find_package(PkgConfig REQUIRED)
pkg_check_modules(YAMLCPP yaml-cpp REQUIRED)
if(YAMLCPP_VERSION VERSION_GREATER "0.5.0")
    add_definitions(-DHAVE_YAMLCPP_GT_0_5_0)
endif()

find_package(yaml-cpp REQUIRED)

include_directories(
	include 
	${OpenCV_INCLUDE_DIRS}
	${YAMLCPP_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
)
link_directories(${OpenCV_LIBRARY_DIRS})
link_directories(${YAMLCPP_LIBRARY_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_executable(label_tools_image_v2 
	src/label_tools_image_v2.cpp
	include/label_tools_image_v2/my_tools.cpp
	include/label_tools_image_v2/my_image.cpp
	include/utils.cpp
)
target_link_libraries(label_tools_image_v2 ${OpenCV_LIBRARIES} ${YAMLCPP_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

add_executable(label_tools_checker 
	src/label_tools_checker.cpp
	include/utils.cpp
)
target_link_libraries(label_tools_checker ${OpenCV_LIBRARIES} ${YAMLCPP_LIBRARIES} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})
