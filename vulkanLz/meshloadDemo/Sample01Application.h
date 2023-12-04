#pragma once
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <array>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
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
	 VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspFlags,
		 uint32_t mipLevels);
	 void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format,
		 VkImageTiling tiling, VkImageUsageFlags usage,
		 VkMemoryPropertyFlags properties, VkImage& image,
		 VkDeviceMemory& imageMemory);
	 VkCommandBuffer beginSingletimeCommands();
	 void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	 void transitionImageLayout(VkImage image, VkFormat format,
		 VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	 void generateMipmaps(VkImage image, int32_t texWidth,
		 int32_t texHeight, VkFormat imageFormat, uint32_t mipLevels);
	 void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
		 width, uint32_t height);
	 void createTextureSampler();
	 void createDepthResources();
	 VkFormat findSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling imgtile,
		 VkFormatFeatureFlags featFlags);
	 VkFormat findDepthFormat();
	 bool hasStencilComponent(VkFormat format) {
		  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format ==
			 VK_FORMAT_D24_UNORM_S8_UINT;
		 
	 }
	 void loadmodel();
public:
	bool isFramebufferResized = false;
	struct vertexIn {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;
		bool operator==(const vertexIn& other) const {
			return pos == other.pos && color == other.color && texCoord ==
				other.texCoord;

		}

	};
private:
	size_t  currentFrame = 0;
	

	std::vector<vertexIn> vertices;
	std::vector<uint32_t> Indices;
	
	struct uniformObject {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Proj;
	} ;


	class GLFWwindow* window;
	 VkInstance  instance;
	const uint32_t  WIDTH = 800;
	const uint32_t  HEIGHT = 600;
	const std::string MODEL_PATH = "models/viking_ware.obj";
	const std::string TEXTURE_PATH = "textures/viking_ware.jpeg";
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
	VkDeviceMemory textureImageMemory;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	
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
	uint32_t mipLevels;
};


namespace std {
	 template<> struct hash<Sample01Application::vertexIn> {
		 size_t operator()(Sample01Application::vertexIn const& vertex) const {
			 return ((hash<glm::vec3>()(vertex.pos) ^
				 (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				 (hash<glm::vec2>()(vertex.texCoord) << 1);
			
		}
		
	};
	
}
