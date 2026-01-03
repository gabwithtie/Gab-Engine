
#GET PATH ABOVE
set(current_dir "${CMAKE_CURRENT_LIST_DIR}")
cmake_path(NORMAL_PATH current_dir)
cmake_path(GET current_dir PARENT_PATH parent_dir)

#load meshoptimizer, a bgfx utility library prerequisite
include(${parent_dir}/bgfx.cmake/cmake/bgfx/3rdparty/meshoptimizer.cmake)

#actually load the bgfx-utils library
set(LIB_NAME "bgfx-utils")
message("[GBE EXTERNAL] Building Library ${LIB_NAME} with ${MESHOPTIMIZER_LIBRARIES} in ${MESHOPTIMIZER_INCLUDE_DIR}")

add_library(${LIB_NAME}
	"${BGFX_DIR}/examples/common/bgfx_utils.cpp"
	${MESHOPTIMIZER_SOURCES}
 "entry_patch.cpp")

target_link_libraries(${LIB_NAME} PUBLIC bgfx bimg bx bimg_decode ${MESHOPTIMIZER_LIBRARIES})

target_include_directories(${LIB_NAME} PUBLIC ${MESHOPTIMIZER_INCLUDE_DIR})
target_include_directories(${LIB_NAME} INTERFACE "${BGFX_DIR}/examples/common")
