# Function for building single example
function(buildExample EXAMPLE_NAME)
	SET(EXAMPLE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/${EXAMPLE_NAME})
	message(STATUS "Generating project file for example in ${EXAMPLE_FOLDER}")
	# Main
	file(GLOB SOURCE *.cpp ${BASE_HEADERS} ${EXAMPLE_FOLDER}/*.cpp)
	SET(MAIN_CPP ${EXAMPLE_FOLDER}/${APP_SOURCE}.cpp)
	if(EXISTS ${EXAMPLE_FOLDER}/main.cpp)
		SET(MAIN_CPP ${EXAMPLE_FOLDER}/main.cpp)
	ENDIF()
	if(EXISTS ${EXAMPLE_FOLDER}/${APP_SOURCE}.h)
		SET(MAIN_HEADER ${EXAMPLE_FOLDER}/${APP_SOURCE}.h)
	ENDIF()
    IF(${EXAMPLE_NAME} STREQUAL "mdlmaterialDemo")
		include_directories(../thirdParty/MDL-SDK/include)
	ENDIF()
	# Add shaders
	
	
	set(SHADER_DIR_HLSL "../data/shaders/${EXAMPLE_NAME}")
	file(GLOB SHADERS_HLSL "${SHADER_DIR_HLSL}/*.vert" "${SHADER_DIR_HLSL}/*.frag" "${SHADER_DIR_HLSL}/*.comp" "${SHADER_DIR_HLSL}/*.geom" "${SHADER_DIR_HLSL}/*.tesc" "${SHADER_DIR_HLSL}/*.tese" "${SHADER_DIR_HLSL}/*.mesh" "${SHADER_DIR_HLSL}/*.task" "${SHADER_DIR_HLSL}/*.rgen" "${SHADER_DIR_HLSL}/*.rchit" "${SHADER_DIR_HLSL}/*.rmiss" "${SHADER_DIR_HLSL}/*.hlsl")

	source_group("Shaders\\HLSL" FILES ${SHADERS_HLSL})
	# Add optional readme / tutorial
	file(GLOB README_FILES "${EXAMPLE_FOLDER}/*.md")
	if(WIN32)
		add_executable(${EXAMPLE_NAME} WIN32 ${MAIN_CPP} ${SOURCE} ${MAIN_HEADER} ${SHADERS_HLSL} ${README_FILES})
		IF(${EXAMPLE_NAME} STREQUAL "mdlmaterialDemo")
		   target_link_libraries(${EXAMPLE_NAME} ${glfw_LIBRARY} ${dxc_LIBRARY} ${Vulkan_LIBRARY} ${WINLIBS})
		   add_definitions(-DUSE_OPENIMAGEIO_PLUGIN=1)
	    ELSE(${EXAMPLE_NAME} STREQUAL "mdlmaterialDemo")
	       target_link_libraries(${EXAMPLE_NAME} ${glfw_LIBRARY} ${Vulkan_LIBRARY} ${WINLIBS})
	    ENDIF(${EXAMPLE_NAME} STREQUAL "mdlmaterialDemo")
	endif(WIN32)

	set_target_properties(${EXAMPLE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data)
	if(OpenMP_CXX_FOUND)
		target_compile_options(${EXAMPLE_NAME} PRIVATE ${OpenMP_CXX_FLAGS})
		IF(${EXAMPLE_NAME} STREQUAL "texture3d")
			if(OpenMP_CXX_FOUND)
    			target_link_libraries(${EXAMPLE_NAME} OpenMP::OpenMP_CXX)
			endif()	
		endif()
	endif()

	if(RESOURCE_INSTALL_DIR)
		install(TARGETS ${EXAMPLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

	endif()
	install(DIRECTORY ../libs/ DESTINATION ${CMAKE_SOURCE_DIR}/bin)
endfunction(buildExample)

# Build all examples
function(buildExamples)
	foreach(EXAMPLE ${EXAMPLES})
		buildExample(${EXAMPLE})
	endforeach(EXAMPLE)
endfunction(buildExamples)

set(EXAMPLES
  triangleDemo
  imageviewDemo
  imageSampler
  depthtestDemo
  meshloadDemo
  multisamplerDemo
  mdlmaterialDemo
)

set(APP_SOURCE Sample01Application)
buildExamples()
