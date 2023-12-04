#pragma once
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <array>
class Sample01Application
{
public:
	void run();
private:
	struct QueueFamilyindices {
		std::optional<uint32_t> graphicFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> transferFamily;
		bool isComplete()
		{
			return graphicFamily.has_value() && presentFamily.has_value() && 
				transferFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentmodes;
	};

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapChainformat(std::vector<VkSurfaceFormatKHR>const& formats);
	VkPresentModeKHR   choosePresentMode(const std::vector< VkPresentModeKHR>& modes);
	VkExtent2D         chooseSwapExtend(const VkSurfaceCapabilitiesKHR& capability);
	void initVulkan();
	void mainLoop();
	void cleanup();
	void initWindow();
	void createInstance();
	bool checkValidateLayerSupport();
	std::vector<const char*> GetRequiredExtensions();

	static VkBool32 VKAPI_CALL UtilsMessengerCallbackEXT(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void SetupDebugMessager();
	static VkResult CreateDebugUtilMessager(VkInstance const& instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pcreateinfo, const VkAllocationCallbacks* pA,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger, const
		VkAllocationCallbacks* pAllocator);

	void popDebugmessageCreateinfo(VkDebugUtilsMessengerCreateInfoEXT& createinfo);
	bool IsDeviceSuitable(const VkPhysicalDevice& device);
	void PickPhysicalDevice();
	 QueueFamilyindices FindQueueFamily(const VkPhysicalDevice& device);
	 void CreateLogicDevice();
	 void destroyLogicalDevice();
	 void createSurface();
	 bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	 void destroySurface();
	 void createSwapChain();
	 void createImageViews();
	 void createGraphicPipeline();
	 VkShaderModule createShaderModule(std::vector<char> const& code);
	 void createRenderPass();
	 void createFramebuffers();
	 void createCommandPool();
	 void createCommandBuffers();
	 void drawFrame();
	 void createSemaphores();
	 void updateUniformBuffer(uint32_t currentImage);
	 void cleanupSwapchain();
	 void recreateSwapchain();
	 void createVertexBuffer();
	 void createIndexBuffer();
	 uint32_t findMemtype(uint32_t typeFilter, VkMemoryPropertyFlags flags);
	 void createTranscommandBuffers();
	 void createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memProp,
		 VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	 void createUniformBuffers();
	 void copyBuffer(VkBuffer srcbuffer, VkBuffer dstbuffer, VkDeviceSize);
	 void createDescsetPool();
	 void createDescriptorSets();
	 void createTextureImage();
	 void createTextureImageView();
	 VkImageView createImageView(VkImage image, VkFormat format);
	 void createImage(uint32_t width, uint32_t height, VkFormat format,
		 VkImageTiling tiling, VkImageUsageFlags usage,
		 VkMemoryPropertyFlags properties, VkImage& image,
		 VkDeviceMemory& imageMemory);
	 VkCommandBuffer beginSingletimeCommands();
	 void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	 void transitionImageLayout(VkImage image, VkFormat format,
		 VkImageLayout oldLayout, VkImageLayout newLayout);
	 void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
		 width, uint32_t height);
	 void createTextureSampler();
public:
	bool isFramebufferResized = false;
private:
	size_t  currentFrame = 0;
	struct vertexIn {
		glm::vec2 pos;
		glm::vec3 color;
		glm::vec2 texCoord;
	};

	std::array< vertexIn, 4> vertices = {
		glm::vec2(-0.5, -0.5),glm::vec3{1.f,0.f,0.f},glm::vec2{0,0},
		glm::vec2{0.5, -0.5},glm::vec3{1.f,1.f,0.f},glm::vec2{1,0},
		glm::vec2(0.5, 0.5),glm::vec3{1.f,1.f,0.f}, glm::vec2{1,1},
		glm::vec2(-0.5, 0.5),glm::vec3{1.f,0.f,0.f}, glm::vec2{0,1} };
	std::array<uint16_t, 6> Indices = {
		0,3,1,1,3,2
	};
	
	struct uniformObject {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Proj;
	} ;


	class GLFWwindow* window;
	 VkInstance  instance;
	const uint32_t  WIDTH = 800;
	const uint32_t  HEIGHT = 600;
    const std::vector<const char*> validateLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> deviceExtensions = {
		        VK_KHR_SWAPCHAIN_EXTENSION_NAME

       };
	static const int  MAX_FRAMES_IN_FLIGHT = 2;

	VkPipelineCache pipelineCache;
	VkDebugUtilsMessengerEXT debugmessager;
	VkPhysicalDevice physicaldevice;
	VkDevice  Device;
	VkQueue  graphicQueue;
	VkSurfaceKHR surface;
	VkQueue  presentQueue;
	VkQueue  transferQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkExtent2D swapchainExtend;
	VkFormat swapchainFormat;
	std::vector<VkImageView> swapChainImageView;
	VkDescriptorSetLayout descriptorSetLayout;

	VkRenderPass    renderPass;
	VkPipelineLayout      pipelineLayout;
	VkPipeline Graphicpipeline;
	std::vector<VkFramebuffer> framebuffers;
	VkCommandPool commandPool;
	VkCommandPool transcommandPool;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkImage textureImage;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkDeviceMemory textureImageMemory;
	std::vector< VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBufMemorys;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandBuffer TransCmdBuffer;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemas;
	std::array<VkSemaphore,MAX_FRAMES_IN_FLIGHT> renderFinishedSemas;
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> InFlightFences;
	std::vector<VkFence> imagesInflight;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	
};

