# vulkanLz

reference the thirdparty libs: glm, mdl-sdk, tiny_obj_load, stb, glfw. 
and the project mdlmaterialDemo's material database is from Nvidia company.
all the shaders are written with HLSL. 

all shaders except mdlmaterialDemo's should be compiled with dxc to spirv shader.
run bash command:
dxc -spirv -T vs/ps_6_6 -E main *.frag/vert/comp -Fo *.spv


