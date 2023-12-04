#include <vulkan/vulkan.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <chrono>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <iostream>
#include "Sample01Application.h"
#include <vector>
#include <algorithm>
#include <ranges>

#include <vulkan/vulkan_win32.h>
#include <array>
#include <set>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


#ifdef NDEBUG
 constexpr bool enableValidationLayers = false;
#else
 constexpr bool enableValidationLayers = true;

#endif


 static std::vector<char> loadFile(const std::string& filename)
 {
	 std::ifstream file(filename, std::ios::ate | std::ios::binary);
	 if (!file.is_open()) {
		 throw std::runtime_error("Failed to open file! ");
	 }
	 size_t filesize = (size_t) file.tellg();
	 std::vector<char> shaderbuffer( filesize );
	 file.seekg(0);
	 file.read(shaderbuffer.data(), filesize);
	 return shaderbuffer;
 }

 static void framebufferResizeCallbck(GLFWwindow* win,int width,int height)
 {
	 Sample01Application* app = reinterpret_cast<Sample01Application*>(
		 glfwGetWindowUserPointer(win));
	 app->isFramebufferResized = true;
 }

void Sample01Application::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(800, 600, "vulkan window",
		nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallbck);
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
		nullptr);
	std::cout << extensionCount << " extension support\n";
}

void Sample01Application::createTextureSampler()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.maxLod = static_cast<float>(mipLevels);
	samplerInfo.minLod = 0;// static_cast<float>(mipLevels / 2);
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	if (VK_SUCCESS != vkCreateSampler(Device, &samplerInfo, nullptr, &textureSampler)) {
		throw std::runtime_error("Failed to create sampler!");
	}
}

VkFormat Sample01Application::findDepthFormat()
{
    return	findSupportedFormat({ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat Sample01Application::findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling imgtile,
	VkFormatFeatureFlags featFlags)
{
	for (auto& format : formats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicaldevice, format, &props);
		if (imgtile == VK_IMAGE_TILING_OPTIMAL) {
			if ((featFlags & props.optimalTilingFeatures) == featFlags)
				return format;
		}
		else if (imgtile == VK_IMAGE_TILING_LINEAR) {
			if ((featFlags & props.linearTilingFeatures) == featFlags) {
				return format;
			}
		}
		
	}
	throw std::runtime_error("Failed to find depth format");

}

void Sample01Application::createTextureImage()
{
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels,
		STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;
	if (!pixels) {
		throw std::runtime_error("Failed to load image file.");
	}
	
	mipLevels =static_cast<uint32_t>( std::floor(std::log2(std::max(texWidth,texHeight)))) + 1;
	size_t buffersize = imageSize;// sizeof(vertices[0])* vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagebufferMem;

	createBuffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagebufferMem);

	void* data;
	vkMapMemory(Device, stagebufferMem, 0, buffersize, 0, &data);
	memcpy(data, pixels, buffersize);
	vkUnmapMemory(Device, stagebufferMem);
	stbi_image_free(pixels);

	//createBuffer(buffersize,
	//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	//	&textureImage, vertexBufferMemory);

	createImage(texWidth, texHeight, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
	copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
	generateMipmaps(textureImage, texWidth, texHeight,VK_FORMAT_R8G8B8A8_SRGB, mipLevels);
	//transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
	//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,mipLevels);
	//copyBuffer(stagingBuffer, vertexBuffer, buffersize);
	vkDestroyBuffer(Device, stagingBuffer, nullptr);
	vkFreeMemory(Device, stagebufferMem, nullptr);

}

void Sample01Application::generateMipmaps(VkImage image, int32_t texWidth,  int32_t
	texHeight, VkFormat imageFormat, uint32_t mipLevels) {
	VkFormatProperties formatProp{};
	vkGetPhysicalDeviceFormatProperties(physicaldevice,imageFormat,&formatProp);
	if (!(formatProp.optimalTilingFeatures &
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) ){
		throw std::runtime_error("texture image format does not support"
			" linear blitting!");
	}
		
	VkCommandBuffer commandBuffer = beginSingletimeCommands();
	 
     VkImageMemoryBarrier barrier{};
	 barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	 barrier.image = image;
	 barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	 barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	 barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	 barrier.subresourceRange.baseArrayLayer = 0;
	 barrier.subresourceRange.layerCount = 1;
	 barrier.subresourceRange.levelCount = 1;
	 int32_t mipWidth = texWidth;
	 int32_t mipHeight = texHeight;
	 
	for (uint32_t i = 1; i < mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		 barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		 barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		 barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		 barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		
	     vkCmdPipelineBarrier(commandBuffer,
				 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				 0, nullptr,
				 0, nullptr,
				 1, &barrier);

		 VkImageBlit blit{};
		  blit.srcOffsets[0] = { 0, 0, 0 };
		  blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		  blit.srcSubresource.mipLevel = i - 1;
		  blit.srcSubresource.baseArrayLayer = 0;
		  blit.srcSubresource.layerCount = 1;
		  blit.dstOffsets[0] = { 0, 0, 0 };
		  blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight >
		 1 ? mipHeight / 2 : 1, 1 };
		  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		  blit.dstSubresource.mipLevel = i;
		  blit.dstSubresource.baseArrayLayer = 0;
		  blit.dstSubresource.layerCount = 1;
		  vkCmdBlitImage(commandBuffer,
			   image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			   1, &blit,
			   VK_FILTER_LINEAR);
		  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		   barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		   barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		   barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		  
			vkCmdPipelineBarrier(commandBuffer,
				   VK_PIPELINE_STAGE_TRANSFER_BIT,
				  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				   0, nullptr,
				   0, nullptr,
				   1, &barrier);
			
			if (mipWidth > 1) mipWidth /= 2;
			 if (mipHeight > 1) mipHeight /= 2;
	 }
	 barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		  
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);
	 endSingleTimeCommands(commandBuffer);
	
}

void Sample01Application::createDepthResources()
{
	VkFormat depthformat = findDepthFormat();
	createImage(swapchainExtend.width, swapchainExtend.height, 1, depthformat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	depthImageView = createImageView(depthImage, depthformat, VK_IMAGE_ASPECT_DEPTH_BIT,1);
	transitionImageLayout(depthImage, depthformat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,1);
}

void Sample01Application::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
	VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties, VkImage& image,
	VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	 imageInfo.imageType = VK_IMAGE_TYPE_2D;
	 imageInfo.extent.width = width;
	 imageInfo.extent.height = height;
	 imageInfo.extent.depth = 1;
	 imageInfo.mipLevels = mipLevels;
	 imageInfo.arrayLayers = 1;
	 imageInfo.format = format;
	 imageInfo.tiling = tiling;
	 imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	 imageInfo.usage = usage;
	 imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	 imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	 vkCreateImage(Device, &imageInfo, nullptr, &image);
	 VkMemoryRequirements MemReq;
	 vkGetImageMemoryRequirements(Device, image, &MemReq);
	 VkMemoryAllocateInfo allocInfo{};
	 allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	 allocInfo.allocationSize = MemReq.size;
	 allocInfo.memoryTypeIndex = findMemtype(MemReq.memoryTypeBits,
		 properties);
	 vkAllocateMemory(Device, &allocInfo, nullptr, &imageMemory);
	 vkBindImageMemory(Device, image, imageMemory, 0);

}

void Sample01Application::loadmodel()
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
			MODEL_PATH.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<vertexIn, uint32_t> uniquedVertex;
	for (const auto& shape : shapes) {
		 for (const auto& index : shape.mesh.indices) {
			 vertexIn vertex{};
			 vertex.pos = {
				 attrib.vertices[3 * index.vertex_index + 0],
			     attrib.vertices[3* index.vertex_index +1],
				 attrib.vertices[3 * index.vertex_index + 2],
			 };
			 vertex.texCoord = {
				 attrib.texcoords[2 * index.texcoord_index + 0],
				 1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			 };
			 vertex.color = { 1.f,1.f,1.f};
			 if (uniquedVertex.count(vertex) == 0) 
			 {
				 uniquedVertex[vertex] = static_cast<uint32_t>(vertices.size());
				 vertices.push_back(vertex);
			 }
		     
			 Indices.push_back(uniquedVertex[vertex]);
		
		}
	}

}

void Sample01Application::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void Sample01Application::createInstance()
{
	if (enableValidationLayers && !checkValidateLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.apiVersion = VK_API_VERSION_1_3;
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 204);
	appInfo.engineVersion = VK_MAKE_VERSION(1, 3, 204);
	appInfo.pApplicationName = "Hello vulkan";
	appInfo.pEngineName = "no engine";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	std::vector<const char*> extensions = GetRequiredExtensions();
	
	VkDebugUtilsMessengerCreateInfoEXT debugcreateinfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = validateLayers.size();
		createInfo.ppEnabledLayerNames = validateLayers.data();
		createInfo.enabledExtensionCount = extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
		popDebugmessageCreateinfo(debugcreateinfo);
		createInfo.pNext =(VkDebugUtilsMessengerCreateInfoEXT*) &debugcreateinfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	 uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> properties(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
		properties.data());
	std::cout << "availble extensions:" << "\n";
	for (auto const& item : properties)
	{
		std::cout << item.extensionName << "\n";
	}
		
}

void Sample01Application::popDebugmessageCreateinfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.flags = 0;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = UtilsMessengerCallbackEXT;
	createInfo.pUserData = nullptr;
	
	createInfo.pNext = nullptr;
}

void Sample01Application::SetupDebugMessager()
{
	if (!enableValidationLayers) {
		return;
	}
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	popDebugmessageCreateinfo(createInfo);
	
	
	if (VK_SUCCESS != CreateDebugUtilMessager(instance, &createInfo, nullptr, &debugmessager))
		throw std::runtime_error("failed to setup messager callback");

}

VkResult Sample01Application::CreateDebugUtilMessager(VkInstance const& instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pcreateinfo, const VkAllocationCallbacks* pA,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance,
		"vkCreateDebugUtilsMessengerEXT");
	
	if (fn != nullptr)
	{
		return fn(instance, pcreateinfo, pA, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Sample01Application::DestroyDebugUtilsMessengerEXT(VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger, const
	VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(instance,
			"vkDestroyDebugUtilsMessengerEXT");

	 if (func != nullptr) {
		 func(instance, debugMessenger, pAllocator);
	}
}

VkBool32 VKAPI_CALL Sample01Application::UtilsMessengerCallbackEXT(
	VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cerr << "validate layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

std::vector<const char*> Sample01Application::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtension;
	glfwExtension = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions{ glfwExtension,glfwExtension +
	glfwExtensionCount }; 
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return extensions;
}

bool Sample01Application::checkValidateLayerSupport()
{
	uint32_t layerCount{ 0 };
	vkEnumerateInstanceLayerProperties(&layerCount,nullptr);
	std::vector<VkLayerProperties> layerProps(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProps.data());
	
	std::ranges::filter_view view{ validateLayers,[&](char const* c)
		{
		    return std::find_if(layerProps.cbegin(),
		           layerProps.cend(),[&](VkLayerProperties const& v) 
				{
			        if (strcmp(v.layerName,c) == 0)
				       return true;
			        else
		               return false; 
				}) != layerProps.cend(); 
		}
	};

	return !view.empty();

}

bool Sample01Application::IsDeviceSuitable(const VkPhysicalDevice& device)
{
	VkPhysicalDeviceProperties devProps;
	VkPhysicalDeviceFeatures   devFeats;
	vkGetPhysicalDeviceProperties(device, &devProps);
	vkGetPhysicalDeviceFeatures(device, &devFeats);
	QueueFamilyindices indices = FindQueueFamily(device);
	bool isextensionsupport = checkDeviceExtensionSupport(device);
	bool isSwapChainAdequate = false;
	if (isextensionsupport) {
		SwapChainSupportDetails details = querySwapChainSupport(device);
		isSwapChainAdequate = !details.formats.empty() && !details.presentmodes.empty();
	}
	return indices.isComplete() && isextensionsupport && isSwapChainAdequate &&
		devFeats.samplerAnisotropy;
}

VkSurfaceFormatKHR Sample01Application::chooseSwapChainformat(std::vector<VkSurfaceFormatKHR>const& formats)
{
	for (auto& item : formats) {
		if (item.format == VK_FORMAT_R8G8B8A8_SRGB &&
			item.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return item;
	}
	return formats[0];
}

VkPresentModeKHR   Sample01Application::choosePresentMode(const std::vector< VkPresentModeKHR>& modes)
{
	for (auto& item : modes)
	{
		if (item == VK_PRESENT_MODE_MAILBOX_KHR)
			return item;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D  Sample01Application::chooseSwapExtend(const VkSurfaceCapabilitiesKHR& capability)
{
	if (capability.currentExtent.width != UINT32_MAX)
		return capability.currentExtent;
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtend{ width, height };
		actualExtend.width = std::max(capability.minImageExtent.width,
			std::min(capability.maxImageExtent.width, actualExtend.width));
		actualExtend.height = std::max(capability.minImageExtent.height,
			std::min(capability.maxImageExtent.height, actualExtend.height));
		return actualExtend;
	}
}

void Sample01Application::createSwapChain()
{
	SwapChainSupportDetails swapchainDetail = querySwapChainSupport(physicaldevice);
	VkExtent2D extend2D = chooseSwapExtend(swapchainDetail.capabilities);
	VkPresentModeKHR presentMode = choosePresentMode(swapchainDetail.presentmodes);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapChainformat(swapchainDetail.formats);
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	uint32_t imagecount = swapchainDetail.capabilities.minImageCount + 1;
	if (imagecount > swapchainDetail.capabilities.maxImageCount && imagecount >0)
	{
		imagecount = swapchainDetail.capabilities.maxImageCount;
	}
	createInfo.minImageCount = imagecount;
	createInfo.surface = surface;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extend2D;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	QueueFamilyindices indices = FindQueueFamily(physicaldevice);
	uint32_t queuefamilyIdx[] = { indices.graphicFamily.value(), indices.presentFamily.value() };
	if (indices.graphicFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queuefamilyIdx;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
	}

	createInfo.preTransform = swapchainDetail.capabilities.currentTransform;
	createInfo.clipped = VK_TRUE;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (VK_SUCCESS != vkCreateSwapchainKHR(Device, &createInfo, nullptr, &swapChain))
	{
		throw std::runtime_error("Failed to create swapchain!");
	}
	uint32_t swapImageCount{ 0 };
	vkGetSwapchainImagesKHR(Device, swapChain, &swapImageCount, nullptr);
	swapChainImages.resize(swapImageCount);
	vkGetSwapchainImagesKHR(Device, swapChain, &swapImageCount, swapChainImages.data());
	swapchainExtend = extend2D;
	swapchainFormat = surfaceFormat.format;
}

void Sample01Application::createTextureImageView()
{
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT,mipLevels);
}

VkImageView Sample01Application::createImageView(VkImage image, VkFormat format,
	VkImageAspectFlags AspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo imageviewInfo{};
	imageviewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageviewInfo.format = format;
	imageviewInfo.image = image;
	
	imageviewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageviewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageviewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageviewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageviewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageviewInfo.subresourceRange.aspectMask = AspectFlags;// VK_IMAGE_ASPECT_COLOR_BIT;
	imageviewInfo.subresourceRange.baseArrayLayer = 0;
	imageviewInfo.subresourceRange.baseMipLevel = 0;
	imageviewInfo.subresourceRange.layerCount = 1;
	imageviewInfo.subresourceRange.levelCount = mipLevels;
	VkImageView imageView;
	vkCreateImageView(Device, &imageviewInfo, nullptr, &imageView);
	return imageView;
}

void Sample01Application::createImageViews()
{
	swapChainImageView.resize(swapChainImages.size());
	int i{ 0 };
	for (auto& image : swapChainImages) {
		swapChainImageView[i++] = createImageView(image, swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT,1);
	}

}

Sample01Application::SwapChainSupportDetails Sample01Application::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	uint32_t formatcount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatcount, nullptr);
	if (formatcount > 0) {
		details.formats.resize(formatcount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatcount, details.formats.data());

	}
	uint32_t presentmodeCount{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentmodeCount, nullptr);
	if (presentmodeCount > 0)
	{
		details.presentmodes.resize(presentmodeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentmodeCount, 
			details.presentmodes.data());

	}
	return details;
}

bool Sample01Application::checkDeviceExtensionSupport(VkPhysicalDevice device) 
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensionprops{ extensionCount };
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensionprops.data());

	
		std::ranges::filter_view filterview{ deviceExtensions , [&](const char* xx) {
			return std::find_if(extensionprops.begin(),extensionprops.end(),
			[&](const VkExtensionProperties& prop) {return std::string_view(xx) == prop.extensionName;
				}) == extensionprops.end();
			} };
	    
	

	return filterview.empty();

}

Sample01Application::QueueFamilyindices Sample01Application::FindQueueFamily(const VkPhysicalDevice& device)
{
	QueueFamilyindices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queuefamilyprops{ queueFamilyCount };
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queuefamilyprops.data());
	int i = 0;
	for (const auto& prop : queuefamilyprops)
	{
		if((prop.queueFlags& VK_QUEUE_GRAPHICS_BIT) &&
			(prop.queueFlags & VK_QUEUE_TRANSFER_BIT))
		{
			indices.graphicFamily = i;
			VkBool32 isPresentSupport;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &isPresentSupport);
			if (isPresentSupport)
			{
				indices.presentFamily = i;
			}
			indices.transferFamily = i;
			//break;
		}
		/*else if (prop.queueFlags & VK_QUEUE_TRANSFER_BIT) {
			
		}*/

		if (indices.isComplete())
			break;
		
		++i;
	}
	return indices;
}

void Sample01Application::CreateLogicDevice()
{
	QueueFamilyindices queuefamilyIdx = FindQueueFamily(physicaldevice);
	std::vector< VkDeviceQueueCreateInfo> devqueuecreateinfos;
	std::set<uint32_t> uniquequeuefamilies = { queuefamilyIdx.graphicFamily.value(),
	queuefamilyIdx.presentFamily.value(), queuefamilyIdx.transferFamily.value()};
	float queuepriority = 1.f;
	for (uint32_t queuefamily : uniquequeuefamilies) {
		VkDeviceQueueCreateInfo devqueuecreateinfo{};
		devqueuecreateinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		devqueuecreateinfo.queueCount = 1;
		devqueuecreateinfo.queueFamilyIndex = queuefamily;
		
		devqueuecreateinfo.pQueuePriorities = &queuepriority;
		devqueuecreateinfos.push_back(devqueuecreateinfo);

	}
	VkPhysicalDeviceFeatures   devFeats{};
	devFeats.samplerAnisotropy = VK_TRUE;
	VkDeviceCreateInfo devcreateinfo{};
	devcreateinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	devcreateinfo.pQueueCreateInfos = devqueuecreateinfos.data();
	devcreateinfo.queueCreateInfoCount = devqueuecreateinfos.size();
	devcreateinfo.pEnabledFeatures = &devFeats;
	devcreateinfo.enabledExtensionCount = deviceExtensions.size();
	devcreateinfo.ppEnabledExtensionNames = deviceExtensions.data();
	if (enableValidationLayers)
	{
		devcreateinfo.enabledLayerCount = validateLayers.size();
		devcreateinfo.ppEnabledLayerNames = validateLayers.data();
	}
	else {
		devcreateinfo.enabledLayerCount = 0;
	}
    
	if (VK_SUCCESS != vkCreateDevice(physicaldevice, &devcreateinfo, nullptr, &Device))
		throw std::runtime_error("Failed to create logical device!");
	
	vkGetDeviceQueue(Device, queuefamilyIdx.graphicFamily.value(), 0, &graphicQueue);
	vkGetDeviceQueue(Device, queuefamilyIdx.presentFamily.value(), 0, &presentQueue);
	vkGetDeviceQueue(Device, queuefamilyIdx.transferFamily.value(), 0, &transferQueue);

}

void Sample01Application::createSurface()
{
	if (VK_SUCCESS != glfwCreateWindowSurface(instance, window, nullptr, &surface))
		throw std::runtime_error("failed to create surface");

}

void Sample01Application::destroySurface()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Sample01Application::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("Failed to get GPU with vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices{ deviceCount };
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	for (auto& device : devices)
	{
		if (IsDeviceSuitable(device)) {
			physicaldevice = device;
			break;
		}
	}

	if(physicaldevice == VK_NULL_HANDLE) {

		throw std::runtime_error("failed to find a suitable GPU!");

			
	}
	
}

VkShaderModule Sample01Application::createShaderModule(std::vector<char> const& code)
{
	VkShaderModuleCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createinfo.codeSize = code.size();
	createinfo.pCode = reinterpret_cast<const uint32_t*>( code.data());
	VkShaderModule shaderModule;
	if (VK_SUCCESS != vkCreateShaderModule(Device, &createinfo, nullptr, &shaderModule))
	{
		throw std::runtime_error("Failed to create shader module!");
	}
	return shaderModule;
}

void Sample01Application::createGraphicPipeline()
{
	auto vertshadercode = loadFile("vertex.spv");
	auto fragshadercode = loadFile("fragment.spv");
	VkShaderModule vertmodule = createShaderModule(vertshadercode);
	VkShaderModule fragmodule = createShaderModule(fragshadercode);
	
	VkPipelineShaderStageCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createinfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	createinfo.module = vertmodule;
	createinfo.pName = "main";
	VkPipelineShaderStageCreateInfo fragcreateinfo{};
	fragcreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragcreateinfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragcreateinfo.module = fragmodule;
	fragcreateinfo.pName = "main";
	std::array< VkPipelineShaderStageCreateInfo, 2> stagecreateinfos{
		createinfo,fragcreateinfo
	};
	
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	VkVertexInputBindingDescription bindingDesc{0,sizeof(vertexIn),VK_VERTEX_INPUT_RATE_VERTEX };

	vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
	std::vector< VkVertexInputAttributeDescription> attrDesc = {
		{0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(vertexIn,pos)},
		{1,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(vertexIn,color)},
		{2,0,VK_FORMAT_R32G32_SFLOAT,offsetof(vertexIn,texCoord)},
	};
	
	vertexInputInfo.pVertexAttributeDescriptions = attrDesc.data();
	vertexInputInfo.vertexAttributeDescriptionCount = attrDesc.size();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	VkPipelineInputAssemblyStateCreateInfo assemblyCreteinfo{};
	assemblyCreteinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyCreteinfo.primitiveRestartEnable = false;
	assemblyCreteinfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkViewport viewport{};
	viewport.x = viewport.y = 0;
	viewport.height = swapchainExtend.height;
	viewport.width = swapchainExtend.width;
	viewport.maxDepth = 1.f;
	viewport.minDepth = 0.f;
	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapchainExtend;
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pViewports = &viewport;
	viewportState.pScissors = &scissor;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.lineWidth = 1.f;
	rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	VkPipelineMultisampleStateCreateInfo MutisampleState{};
	MutisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	MutisampleState.sampleShadingEnable = VK_FALSE;
	MutisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineColorBlendAttachmentState colorblendstate{};
	colorblendstate.blendEnable = VK_TRUE;
	colorblendstate.colorBlendOp = VK_BLEND_OP_ADD;
	colorblendstate.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorblendstate.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorblendstate.alphaBlendOp = VK_BLEND_OP_ADD;
	colorblendstate.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorblendstate.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorblendstate.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;

	VkPipelineColorBlendStateCreateInfo blendcreateinfo{};
	blendcreateinfo.attachmentCount = 1;
	blendcreateinfo.pAttachments = &colorblendstate;
	blendcreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendcreateinfo.logicOpEnable = VK_FALSE;
	
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerlayoutBinding{};
	samplerlayoutBinding.binding = 1;
	samplerlayoutBinding.descriptorCount = 1;
	samplerlayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerlayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerlayoutBinding.pImmutableSamplers = nullptr;
	std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{ layoutBinding,samplerlayoutBinding };

	VkDescriptorSetLayoutCreateInfo descsetlayoutInfo{};
	descsetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descsetlayoutInfo.bindingCount = 2;
	descsetlayoutInfo.pBindings = layoutBindings.data();
	descsetlayoutInfo.pNext = nullptr;

	vkCreateDescriptorSetLayout(Device, &descsetlayoutInfo, nullptr, &descriptorSetLayout);
	
	VkPipelineLayoutCreateInfo layoutcreateinfo{};
	layoutcreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutcreateinfo.pSetLayouts = &descriptorSetLayout;
	layoutcreateinfo.setLayoutCount = 1;
	layoutcreateinfo.pPushConstantRanges = nullptr;
	layoutcreateinfo.pushConstantRangeCount = 0;
	if (VK_SUCCESS != vkCreatePipelineLayout(Device, &layoutcreateinfo, nullptr, &pipelineLayout))
	{
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	//VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	//pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	//vkCreatePipelineCache(Device, &pipelineCacheCreateInfo, nullptr, &pipelineCache);
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	 depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	 depthStencil.depthTestEnable = VK_TRUE;
	 depthStencil.depthWriteEnable = VK_TRUE;
	 depthStencil.depthBoundsTestEnable = VK_FALSE;
	 depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	 depthStencil.maxDepthBounds = 1.f;
	 depthStencil.minDepthBounds = 0.f;
	 depthStencil.stencilTestEnable = VK_FALSE;
	 

	VkGraphicsPipelineCreateInfo GraphicPipelineInfo{};
	GraphicPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	GraphicPipelineInfo.layout = pipelineLayout;
	GraphicPipelineInfo.pColorBlendState = &blendcreateinfo;
	GraphicPipelineInfo.pVertexInputState = &vertexInputInfo;
	GraphicPipelineInfo.pRasterizationState = &rasterizationInfo;
	GraphicPipelineInfo.pViewportState = &viewportState;
	GraphicPipelineInfo.pMultisampleState = &MutisampleState;
	GraphicPipelineInfo.renderPass = renderPass;
	GraphicPipelineInfo.stageCount = 2;
	GraphicPipelineInfo.pStages = stagecreateinfos.data();
	GraphicPipelineInfo.pInputAssemblyState = &assemblyCreteinfo;
	GraphicPipelineInfo.subpass = 0;
	GraphicPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	GraphicPipelineInfo.basePipelineIndex = -1;
	GraphicPipelineInfo.pDepthStencilState = &depthStencil;
	GraphicPipelineInfo.pDynamicState = nullptr;
	GraphicPipelineInfo.pTessellationState = nullptr;
		
	if (VK_SUCCESS != vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1,
		&GraphicPipelineInfo,
		nullptr,
		&Graphicpipeline))
	{
		throw std::runtime_error("failed to create pipeline!");
	}
	vkDestroyShaderModule(Device, vertmodule, nullptr);
	vkDestroyShaderModule(Device, fragmodule, nullptr);
}


void Sample01Application::createRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference colorattachmentRef{};
	colorattachmentRef.attachment = 0;
	colorattachmentRef.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorattachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency  subpassDepend{};
	subpassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDepend.dstSubpass = 0;
	subpassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDepend.srcAccessMask = 0;
	subpassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments{ colorAttachment,depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies =  &subpassDepend;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	if (VK_SUCCESS != vkCreateRenderPass(Device, &renderPassInfo, nullptr, &renderPass))
	{
		throw std::runtime_error("Failed to create renderpass!");
	}
}

void Sample01Application::createCommandPool()
{
	QueueFamilyindices indices = FindQueueFamily(physicaldevice);
	VkCommandPoolCreateInfo createinfo{};
	createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createinfo.queueFamilyIndex = indices.graphicFamily.value();
	if (VK_SUCCESS != vkCreateCommandPool(Device, &createinfo, nullptr, &commandPool)) {
		throw std::runtime_error("Failed to create command pool!");
	}
	VkCommandPoolCreateInfo cmdpoolInfo{};
	cmdpoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdpoolInfo.queueFamilyIndex = indices.transferFamily.value();
	cmdpoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	if (VK_SUCCESS != vkCreateCommandPool(Device, &cmdpoolInfo, nullptr, &transcommandPool)) {
		throw std::runtime_error("Failed to create transfer command pool!");
	}
  
}

void Sample01Application::destroyLogicalDevice()
{
	vkDestroyDevice(Device, nullptr);
}

void Sample01Application::createFramebuffers()
{
	framebuffers.resize(swapChainImageView.size());
	int i{ 0 };
	for (auto& imageview : swapChainImageView) {
		VkImageView attachments[] = { imageview, depthImageView };
		VkFramebufferCreateInfo framebuffInfo{};
		framebuffInfo.attachmentCount = 2;
		framebuffInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffInfo.pAttachments = attachments;
		framebuffInfo.renderPass = renderPass;
		framebuffInfo.height = swapchainExtend.height;
		framebuffInfo.width = swapchainExtend.width;
		framebuffInfo.layers = 1;
		if (VK_SUCCESS != vkCreateFramebuffer(Device, &framebuffInfo, nullptr, &framebuffers[i++]))
		{
			throw std::runtime_error("Failed to create frame buffer!");
		}
		
	}
}

void Sample01Application::initVulkan()
{
	createInstance();
	SetupDebugMessager();
	createSurface();
	PickPhysicalDevice();
	CreateLogicDevice();
	createSwapChain();
	createImageViews();
	
	createRenderPass();
	

	createGraphicPipeline();
	
	createCommandPool();
	createTextureImage();
	createDepthResources();
	createFramebuffers();
	
	createTextureImageView();
	createTextureSampler();
	loadmodel();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffers();
		
	createDescsetPool();
	createDescriptorSets();
	createCommandBuffers();
	createSemaphores();
	
}

void Sample01Application::updateUniformBuffer(uint32_t currentImage)
{
	static auto starttime = std::chrono::high_resolution_clock::now();
	auto currenttime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(
		currenttime - starttime	).count();

	uniformObject ubo{};
	ubo.Model = glm::rotate(glm::mat4(1.0),glm::radians(-45.f),
		glm::vec3(0.,0.,1.));

	ubo.View =  glm::lookAt(glm::vec3(2.0f, 2.0f, 8.0f), glm::vec3(0.0f,
		0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.Proj = glm::perspective(glm::radians(45.f),
		swapchainExtend.width / (float)swapchainExtend.height, 0.1f, 100.f);
//	ubo.Proj[1][1] *= -1;
	void* data;
	vkMapMemory(Device, uniformBufMemorys[currentImage], 0,	sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(Device, uniformBufMemorys[currentImage]);

}

void Sample01Application::createDescsetPool()
{
	std::array<VkDescriptorPoolSize,2> poolsizes{};
	poolsizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolsizes[0].descriptorCount = swapChainImages.size();
	poolsizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolsizes[1].descriptorCount = swapChainImages.size();
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolsizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
	vkCreateDescriptorPool(Device, &poolInfo, nullptr, &descriptorPool);
		
}

void Sample01Application::createDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts{ swapChainImages.size(),descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
	allocInfo.pSetLayouts = layouts.data();
	descriptorSets.resize(swapChainImages.size());
	if (VK_SUCCESS != vkAllocateDescriptorSets(Device, &allocInfo, descriptorSets.data()))
	{
		throw std::runtime_error("Failed to allocate desc sets!");
	}
	
	for (int i = 0; i < swapChainImages.size(); ++i) {
		VkDescriptorBufferInfo descBufferInfo{};
		descBufferInfo.buffer = uniformBuffers[i];
		descBufferInfo.offset = 0;
		descBufferInfo.range = sizeof(uniformObject);
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;
		
		std::array<VkWriteDescriptorSet ,2> descWrites{};
		descWrites[0].descriptorCount = 1;
		descWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descWrites[0].dstArrayElement = 0;
		descWrites[0].dstBinding = 0;
		descWrites[0].dstSet = descriptorSets[i];
		descWrites[0].pBufferInfo = &descBufferInfo;
	//	descWrites[0].pImageInfo = nullptr;
		//descWrites[0].pTexelBufferView = nullptr;
		descWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		descWrites[1].descriptorCount = 1;
		descWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descWrites[1].dstArrayElement = 0;
		descWrites[1].dstBinding = 1;
		descWrites[1].dstSet = descriptorSets[i];
	//	descWrites[1].pBufferInfo = nullptr;
		descWrites[1].pImageInfo = &imageInfo;
		//descWrites[1].pTexelBufferView = nullptr;
		descWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		vkUpdateDescriptorSets(Device, descWrites.size(), descWrites.data(), 0, nullptr);
	}
}

void Sample01Application::createSemaphores()
{
	VkSemaphoreCreateInfo semaphCreateInfo{};
	semaphCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	imagesInflight.resize(swapChainImages.size(), VK_NULL_HANDLE);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(Device, &semaphCreateInfo, nullptr, &imageAvailableSemas[i]) != VK_SUCCESS ||
			vkCreateSemaphore(Device, &semaphCreateInfo, nullptr, &renderFinishedSemas[i]) != VK_SUCCESS ||
			vkCreateFence(Device,&fenceCreateInfo,nullptr,&InFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create synchronization object for frame!");
		}
	}
}

void Sample01Application::drawFrame()
{
	uint32_t imageidx;
	vkWaitForFences(Device, 1, &InFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	VkResult result = vkAcquireNextImageKHR(Device, swapChain, UINT64_MAX, imageAvailableSemas[currentFrame], VK_NULL_HANDLE,
		&imageidx);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire image!");

	}
	if (imagesInflight[imageidx] != VK_NULL_HANDLE) {
		vkWaitForFences(Device, 1, &imagesInflight[currentFrame], VK_TRUE, UINT64_MAX);
		
	}
	imagesInflight[imageidx] = InFlightFences[currentFrame];
	updateUniformBuffer(imageidx);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageidx];
	VkSemaphore signalSemaphore[] = { renderFinishedSemas[currentFrame]};
	submitInfo.pSignalSemaphores = signalSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	VkPipelineStageFlags stageFlag[]={ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = stageFlag;
	VkSemaphore waitSemaphore[] = { imageAvailableSemas[currentFrame]};
	submitInfo.pWaitSemaphores = waitSemaphore;
	submitInfo.waitSemaphoreCount = 1;
	vkResetFences(Device, 1, &InFlightFences[currentFrame]);
	if (VK_SUCCESS != vkQueueSubmit(graphicQueue, 1, &submitInfo, InFlightFences[currentFrame]))
	{
		throw std::runtime_error("Failed to queue submit!");
	}
	VkPresentInfoKHR presentinfo{};
	presentinfo.pImageIndices = &imageidx;
	presentinfo.pSwapchains = &swapChain;
	presentinfo.pWaitSemaphores = signalSemaphore;
	presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentinfo.swapchainCount = 1;
	presentinfo.waitSemaphoreCount = 1;
	result = vkQueuePresentKHR(presentQueue, &presentinfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || 
		isFramebufferResized) {
		recreateSwapchain();
		isFramebufferResized = false;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present image!");
	}
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Sample01Application::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawFrame();
	}
}

void Sample01Application::recreateSwapchain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window,&width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(Device);
	cleanupSwapchain();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicPipeline();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescsetPool();
	createDescriptorSets();
	createCommandBuffers();
}

void Sample01Application::cleanupSwapchain()
{
	vkDeviceWaitIdle(Device);
	vkDestroyImageView(Device, depthImageView, nullptr);
	vkDestroyImage(Device, depthImage, nullptr);
	vkFreeMemory(Device, depthImageMemory, nullptr);
	for (auto& framebuff : framebuffers)
	{
		vkDestroyFramebuffer(Device, framebuff, nullptr);
	}
	vkFreeCommandBuffers(Device, commandPool, commandBuffers.size(),
		commandBuffers.data());

	vkDestroyDescriptorSetLayout(Device, descriptorSetLayout, nullptr);

	vkDestroyPipeline(Device, Graphicpipeline, nullptr);
	vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
	vkDestroyRenderPass(Device, renderPass, nullptr);
	for (auto& imageview : swapChainImageView)
	{
		vkDestroyImageView(Device, imageview, nullptr);
	}

	for (int i = 0; i < swapChainImages.size(); ++i)
	{
		vkDestroyBuffer(Device, uniformBuffers[i], nullptr);
		vkFreeMemory(Device, uniformBufMemorys[i], nullptr);
	}
	
	vkDestroyDescriptorPool(Device, descriptorPool, nullptr);

	vkDestroySwapchainKHR(Device, swapChain, nullptr);
}

void Sample01Application::cleanup()
{
	
	cleanupSwapchain();
	vkDestroySampler(Device, textureSampler, nullptr);
	vkDestroyImageView(Device, textureImageView, nullptr);

	vkDestroyImage(Device, textureImage, nullptr);
	vkFreeMemory(Device, textureImageMemory, nullptr);
	vkDestroyBuffer(Device, indexBuffer, nullptr);
	vkFreeMemory(Device, indexBufferMemory, nullptr);
	vkDestroyBuffer(Device, vertexBuffer, nullptr);
	vkFreeMemory(Device, vertexBufferMemory, nullptr);
			
	vkDestroyCommandPool(Device, commandPool, nullptr);
	vkDestroyCommandPool(Device, transcommandPool, nullptr);
	
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroySemaphore(Device, imageAvailableSemas[i], nullptr);
		vkDestroySemaphore(Device, renderFinishedSemas[i], nullptr);
		vkDestroyFence(Device, InFlightFences[i], nullptr);
	}
 /*   for (auto& framebuff : framebuffers)
	{
		vkDestroyFramebuffer(Device, framebuff, nullptr);
	}*/
	//vkDestroyPipeline(Device, Graphicpipeline, nullptr);
	//vkDestroyPipelineLayout(Device, pipelineLayout, nullptr);
	//vkDestroyRenderPass(Device, renderPass, nullptr);
	
	//for (auto& imageview : swapChainImageView)
	//{
	//	vkDestroyImageView(Device,imageview,nullptr);
	//}
	
	//vkDestroySwapchainKHR(Device, swapChain, nullptr);
	
	destroyLogicalDevice();
	destroySurface();
	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(instance, debugmessager, nullptr);
	
	
	vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
	glfwTerminate();
}

uint32_t Sample01Application::findMemtype(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties memprops;
	vkGetPhysicalDeviceMemoryProperties(physicaldevice, &memprops);
	for (int i = 0; i < memprops.memoryTypeCount; ++i) {
		if( (typeFilter & (1 << i))&&
			(memprops.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");

}

void Sample01Application::createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memProp,
	VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;//
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (VK_SUCCESS != vkCreateBuffer(Device, &bufferCreateInfo, nullptr, &buffer)) {
		throw std::runtime_error("Failed to create buffer.");
	}
	VkMemoryRequirements memReq{};
	vkGetBufferMemoryRequirements(Device, buffer, &memReq);
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemtype(memReq.memoryTypeBits,
		memProp);
	if (VK_SUCCESS != vkAllocateMemory(Device, &allocInfo, nullptr, &bufferMemory))
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}
	
	vkBindBufferMemory(Device, buffer, bufferMemory, 0);
}

void Sample01Application::createIndexBuffer()
{
	size_t buffersize = sizeof(Indices[0]) * Indices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagebufferMem;

	createBuffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagebufferMem);

	void* data;
	vkMapMemory(Device, stagebufferMem, 0, buffersize, 0, &data);
	memcpy(data, Indices.data(), buffersize);
	vkUnmapMemory(Device, stagebufferMem);

	createBuffer(buffersize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer, indexBufferMemory);

	copyBuffer(stagingBuffer, indexBuffer, buffersize);
	vkDestroyBuffer(Device, stagingBuffer, nullptr);
	vkFreeMemory(Device, stagebufferMem, nullptr);
}

void Sample01Application::createVertexBuffer()
{
	size_t buffersize = sizeof(vertices[0]) * vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagebufferMem;

	createBuffer(buffersize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagebufferMem);
	
	void* data;
	vkMapMemory(Device, stagebufferMem, 0, buffersize, 0, &data);
	memcpy(data, vertices.data(), buffersize);
	vkUnmapMemory(Device, stagebufferMem);

	createBuffer(buffersize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer, vertexBufferMemory);

	copyBuffer(stagingBuffer, vertexBuffer, buffersize);
	vkDestroyBuffer(Device, stagingBuffer, nullptr);
	vkFreeMemory(Device, stagebufferMem, nullptr);

}

void Sample01Application::copyBuffer(VkBuffer srcbuffer, VkBuffer dstbuffer, VkDeviceSize sz)
{
	//createTranscommandBuffers();
	VkCommandBuffer commandbuffer = beginSingletimeCommands();
	VkBufferCopy copyRegion{};
	copyRegion.size = sz;
	vkCmdCopyBuffer(commandbuffer, srcbuffer, dstbuffer, 1, &copyRegion);
	endSingleTimeCommands(commandbuffer);
	/*vkEndCommandBuffer(TransCmdBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &TransCmdBuffer;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);
	vkFreeCommandBuffers(Device, transcommandPool, 1, &TransCmdBuffer);*/
	
}

void Sample01Application::transitionImageLayout(VkImage image, VkFormat format,
	VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkCommandBuffer commandbuffer = beginSingletimeCommands();
	VkImageMemoryBarrier barrior{};
	barrior.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrior.oldLayout = oldLayout;
	barrior.newLayout = newLayout;
	barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrior.image = image;
	barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
	{
		 barrior.srcAccessMask = 0;
		 barrior.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		 sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		 destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
	{
		barrior.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrior.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout ==
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrior.srcAccessMask = 0;
		barrior.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		
	}
	else 
	{
		 throw std::invalid_argument("unsupported layout transition!");
	}
	
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrior.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (hasStencilComponent(format)) {
			barrior.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrior.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	
	barrior.subresourceRange.baseMipLevel = 0;
	barrior.subresourceRange.levelCount = mipLevels;
	barrior.subresourceRange.baseArrayLayer = 0;
	barrior.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(commandbuffer, sourceStage, destinationStage,
		0, 0, nullptr, 0, nullptr, 1, &barrior);
	
	endSingleTimeCommands(commandbuffer);
}

void Sample01Application::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
	width, uint32_t height)
{
	VkCommandBuffer cmdbuffer= beginSingletimeCommands();
	VkBufferImageCopy region{};
	region.bufferImageHeight = 0;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.imageExtent = { width,height,1 };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageSubresource.mipLevel = 0;
	region.imageOffset = { 0,0,0 };
	vkCmdCopyBufferToImage(cmdbuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region);

	endSingleTimeCommands(cmdbuffer);
}

VkCommandBuffer Sample01Application::beginSingletimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = transcommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkCommandBuffer commandbuffer;
	if (VK_SUCCESS != vkAllocateCommandBuffers(Device, &allocInfo, &commandbuffer))
	{
		throw std::runtime_error("Failed to allocate command buffer!");
	}
	int i = 0;
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(commandbuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin command buffer!");
		}
	}
	return commandbuffer;
}

void Sample01Application::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);
	VkSubmitInfo submitInfo{};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);
	vkFreeCommandBuffers(Device, transcommandPool, 1, &commandBuffer);
}

void Sample01Application::createTranscommandBuffers()
{
	//commandBuffers.resize(framebuffers.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.commandPool = transcommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (VK_SUCCESS != vkAllocateCommandBuffers(Device, &allocInfo, &TransCmdBuffer))
	{
		throw std::runtime_error("Failed to allocate command buffer!");
	}
	int i = 0;
	 {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (vkBeginCommandBuffer(TransCmdBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin command buffer!");
		}
	 }
}

void Sample01Application::createUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(uniformObject);
	uniformBuffers.resize(swapChainImages.size());
	uniformBufMemorys.resize(swapChainImages.size());
	for (int i = 0; i < swapChainImages.size(); ++i) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i], uniformBufMemorys[i]);

	}
	
}

void Sample01Application::createCommandBuffers()
{
	commandBuffers.resize(framebuffers.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = commandBuffers.size();
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if (VK_SUCCESS != vkAllocateCommandBuffers(Device, &allocInfo, commandBuffers.data()))
	{
		throw std::runtime_error("Failed to allocate command buffer!");
	}
	int i = 0;
	for (auto& commandbufffer : commandBuffers) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pInheritanceInfo = nullptr;
		beginInfo.flags = 0;
		if (vkBeginCommandBuffer(commandbufffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin command buffer!");
		}
		VkRenderPassBeginInfo passBeginInfo{};
		passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passBeginInfo.framebuffer = framebuffers[i];
		
		std::array<VkClearValue, 2>  clearcolor{};
		clearcolor[0].color = {0.f,0.f,0.f,1.f};
		clearcolor[1].depthStencil = { 1,0 };
		passBeginInfo.pClearValues = clearcolor.data();
		passBeginInfo.renderArea.extent = swapchainExtend;
		passBeginInfo.renderArea.offset =VkOffset2D { 0,0 };
		passBeginInfo.renderPass = renderPass;
		passBeginInfo.clearValueCount = 2;
		vkCmdBeginRenderPass(commandbufffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandbufffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Graphicpipeline);
		VkBuffer vBuffer[] = { vertexBuffer };
		VkDeviceSize vsize[] = { 0 };
		vkCmdBindVertexBuffers(commandbufffer, 0, 1, vBuffer, vsize);
		vkCmdBindIndexBuffer(commandbufffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		//vkCmdDraw(commandbufffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdBindDescriptorSets(commandbufffer,VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
		
		vkCmdDrawIndexed(commandbufffer, static_cast<uint32_t>(Indices.size()), 1,
			0, 0, 0);
		vkCmdEndRenderPass(commandbufffer);
		if (VK_SUCCESS != vkEndCommandBuffer(commandbufffer))
		{
			throw std::runtime_error("Failed to execute command buffer!");
		}
		++i;
	}
		
}