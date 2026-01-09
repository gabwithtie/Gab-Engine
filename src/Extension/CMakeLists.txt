set(CURRENT_CMAKE_LIB gbe_extension)

#SUBDIRECTORY LOADING
add_library(${CURRENT_CMAKE_LIB} STATIC
	"Extension.h"	
 "Extension.cpp")

target_link_libraries(${CURRENT_CMAKE_LIB} PRIVATE gbe_editor)
message("[GABENGINE] LOADED: " ${CMAKE_CURRENT_SOURCE_DIR})