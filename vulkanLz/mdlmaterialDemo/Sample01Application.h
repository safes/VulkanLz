#pragma once

#include <mi/mdl_sdk.h>
#include <direct.h>
//#pragma pop_macro("malloc")
//#pragma pop_macro("free")
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <array>
#include <fstream>
#include <stack>
#include <iostream>
extern HMODULE g_dso_handle;
namespace mi {
	namespace lz {
		namespace strings {
			template<typename  ... Args>
			inline std::string format(const char* format_str, Args ... args) {
				int size = 1 + snprintf(nullptr, 0, format_str,
					std::forward<Args...>(args)...);
				std::string s;
				s.resize(size);
				snprintf(&s[0], size, format_str, std::forward<Args...>(args)...);
				return s.substr(0, size - 1);;
			}
			inline std::string replace(
				const std::string& input,
				const std::string& old,
				const std::string& with)
			{
				if (input.empty()) return input;

				std::string result(input);
				size_t offset(0);
				while (true)
				{
					size_t pos = result.find(old, offset);
					if (pos == std::string::npos)
						break;

					result.replace(pos, old.length(), with);
					offset = pos + with.length();
				}
				return result;
			}
			inline bool ends_with(const std::string& s, const std::string& potential_end)
			{
				size_t n = potential_end.size();
				size_t sn = s.size();
				for (size_t i = 0; i < n; ++i) {
					if (s[sn - i - 1] != potential_end[n - i - 1])
						return false;
				}
				return true;
			}

			inline std::string to_string(mi::base::Message_severity severity)
			{
				switch (severity)
				{
				case mi::base::MESSAGE_SEVERITY_FATAL:
					return "fatal";
				case mi::base::MESSAGE_SEVERITY_ERROR:
					return "error";
				case mi::base::MESSAGE_SEVERITY_WARNING:
					return "warning";
				case mi::base::MESSAGE_SEVERITY_INFO:
					return "info";
				case mi::base::MESSAGE_SEVERITY_VERBOSE:
					return "verbose";
				case mi::base::MESSAGE_SEVERITY_DEBUG:
					return "debug";
				default:
					break;
				}
				return "";
			}

			inline std::string wstr_to_str(const std::wstring& wstr)
			{
				//using convert_type = std::codecvt<wchar_t,char,mbstate_t>;
				std::string s;
				int sz = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
					nullptr, 0, nullptr, nullptr);
				if (0 == sz) return "";

				s.resize(sz);
				WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
					s.data(), sz, nullptr, nullptr);

				//std::wstring_convert<convert_type, wchar_t> converter;
				return s;
			}

			inline std::wstring str_to_wstr(const std::string& str)
			{
				using convert_type = std::codecvt<wchar_t, char, mbstate_t>;
				std::wstring s;
				int sz = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
				s.resize(sz);
				MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, s.data(), sz);
				return s;
			}

			inline std::string to_string(mi::neuraylib::IMessage::Kind message_kind)
			{
				switch (message_kind) {
				case mi::neuraylib::IMessage::MSG_INTEGRATION:
					return "MDL SDK";
				case mi::neuraylib::IMessage::MSG_IMP_EXP:
					return "Importer/Exporter";
				case mi::neuraylib::IMessage::MSG_COMILER_BACKEND:
					return "Compiler Backend";
				case mi::neuraylib::IMessage::MSG_COMILER_CORE:
					return "Compiler Core";
				case mi::neuraylib::IMessage::MSG_COMPILER_ARCHIVE_TOOL:
					return "Compiler Archive Tool";
				case mi::neuraylib::IMessage::MSG_COMPILER_DAG:
					return "Compiler DAG generator";
				default:
					break;

				}
				return "";
			}

			inline bool starts_with(const std::string& filename, const std::string& prefix)
			{
				size_t n = prefix.size();
				if (n > filename.size())
					return false;

				for (int i = 0; i < n; ++i) {
					if (filename[i] != prefix[i])
						return false;
				}
				return true;
			}

			inline std::vector<std::string> split(
				const std::string& input,
				char sep)
			{
				std::vector<std::string> chunks;
				std::string chunk;
				size_t offset(0);
				size_t pos(0);
				while (pos != std::string::npos) {
					pos = input.find(sep, offset);
					if (pos == std::string::npos) {
						chunk = input.substr(offset);
					}
					else
						chunk = input.substr(offset, pos - offset);
					if (!chunk.empty())
						chunks.push_back(chunk);

					offset++;
				}
				return chunks;

			}
		}

		namespace io {
			inline 	std::string normalize(std::string path, bool remove_dir_ups = false)
			{
				if (path.empty())
					return path;
#ifdef _WIN32
				std::replace(path.begin(), path.end(), '\\', '/');
#endif
				if (remove_dir_ups) {
					bool isAbsuletly = path[0] == '/';
					bool isAbsuleteUNC = path[1] == '/' && isAbsuletly;
					std::vector<std::string> chunks = mi::lz::strings::split(path, '/');
					std::stack<std::string> pathstack;
					for (const std::string& c : chunks)
					{
						if (c.empty() || c == ".")
							continue;
						if (c == "..") {
							if (pathstack.empty())
							{
								pathstack.push(c);
							}
							else {
								pathstack.pop();
							}
							continue;
						}

						pathstack.push(c);
					}
					if (pathstack.empty()) return "";

					path = pathstack.top();
					pathstack.pop();
					while (!pathstack.empty()) {
						path = pathstack.top() + "/" + path;
						pathstack.pop();
					}
					if (isAbsuletly) {
						path = "/" + path;
					}
					if (isAbsuleteUNC) {
						path = "/" + path;
					}
				}
				return path;
			}

			inline std::string read_text_file(const std::string& filename)
			{
				std::ifstream file(filename.c_str());

				if (!file.is_open())
				{
					std::cerr << "Cannot open file: \"" << filename << "\".\n";
					return "";
				}

				std::stringstream string_stream;
				string_stream << file.rdbuf();

				return string_stream.str();
			}

			inline bool is_absolute_path(std::string const& path)
			{
				std::string norPath = normalize(path);
#ifdef _WIN32
				if (norPath.size() < 2) {
					return false;
				}
				else if (norPath[0] == '/' && norPath[1] == '/')
					return true;
				else if (isalpha(norPath[0]) && norPath[1] == '/')
					return true;
				return false;
#else 
				return norPath[0] == '/';
#endif

			}

			inline std::string get_working_directory()
			{
				char current_path[FILENAME_MAX];
#ifdef _WIN32
				_getcwd(current_path, FILENAME_MAX);
#else
				getcwd(current_path, FILENAME_MAX); // TODO
#endif
				return normalize(current_path);
			}

			inline std::string get_executable_folder()
			{
#ifdef _WIN32
				char path[MAX_PATH];
				if (!GetModuleFileNameA(nullptr, path, MAX_PATH))
					return "";

				const char sep = '\\';
#else  // MI_PLATFORM_WINDOWS
				char path[4096];

#ifdef MI_PLATFORM_MACOSX
				uint32_t buflen(sizeof(path));
				if (_NSGetExecutablePath(path, &buflen) != 0)
					return "";
#else  // MI_PLATFORM_MACOSX
				char proc_path[64];
#ifdef __FreeBSD__
				snprintf(proc_path, sizeof(proc_path), "/proc/%d/file", getpid());
#else
				snprintf(proc_path, sizeof(proc_path), "/proc/%d/exe", getpid());
#endif

				ssize_t written = readlink(proc_path, path, sizeof(path));
				if (written < 0 || size_t(written) >= sizeof(path))
					return "";
				path[written] = 0;  // add terminating null
#endif // MI_PLATFORM_MACOSX

				const char sep = '/';
#endif // MI_PLATFORM_WINDOWS

				char* last_sep = strrchr(path, sep);
				if (last_sep == nullptr) return "";
				return std::string(path, last_sep);
			}

		}

		namespace mdl {
			
			inline std::string add_missing_material_signature(const mi::neuraylib::IModule* module,
				const std::string& material_name)
			{
				// Return input if it already contains a signature.
				if (material_name.back() == ')')
				{
					return material_name;
				}

				mi::base::Handle<const mi::IArray> result(module->get_function_overloads(material_name.c_str()));

				// Not supporting multiple function overloads with the same name but different signatures.
				if (!result || result->get_length() != 1)
				{
					return std::string();
				}

				mi::base::Handle<const mi::IString> overloads(result->get_element<mi::IString>(static_cast<mi::Size>(0)));

				return overloads->get_c_str();
			}
			
			inline bool unload()
			{
#ifdef _WIN32
				BOOL result = FreeLibrary(g_dso_handle);
				if (!result) {
					LPTSTR buffer = 0;
					LPCTSTR message = TEXT("unknown failure");
					DWORD error_code = GetLastError();
					if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
						FORMAT_MESSAGE_IGNORE_INSERTS, 0, error_code,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buffer, 0, 0))
						message = buffer;
					//fprintf(stderr, "Failed to unload library (%u): " FMT_LPTSTR, error_code, message);
					if (buffer)
						LocalFree(buffer);
					return false;
				}
				return true;
#else
				int result = dlclose(g_dso_handle);
				if (result != 0) {
					printf("%s\n", dlerror());
					return false;
				}
				return true;
#endif
			}

		}
	}
}

struct Vulkan_texture
{
	VkImage image = nullptr;
	VkImageView image_view = nullptr;
	VkDeviceMemory device_memory = nullptr;

	void destroy(VkDevice device)
	{
		vkDestroyImageView(device, image_view, nullptr);
		vkDestroyImage(device, image, nullptr);
		vkFreeMemory(device, device_memory, nullptr);
	}
};

struct Vulkan_buffer
{
	VkBuffer buffer = nullptr;
	VkDeviceMemory device_memory = nullptr;

	void destroy(VkDevice device)
	{
		vkDestroyBuffer(device, buffer, nullptr);
		vkFreeMemory(device, device_memory, nullptr);
	}
};



class Sample01Application
{
public:
	void run();
	Sample01Application(mi::base::Handle<mi::neuraylib::ITransaction> transaction,
		mi::base::Handle<mi::neuraylib::IMdl_impexp_api> mdl_impexp_api,
		mi::base::Handle<mi::neuraylib::IImage_api> image_api,
		mi::base::Handle<const mi::neuraylib::ITarget_code> target_code):
		m_transaction(transaction),m_mdl_impexp_api(mdl_impexp_api),
		m_image_api(image_api),m_target_code(target_code)
	{
		m_user_data.animation_time = 0.0f;
		m_user_data.material_pattern = 1;
	}
private:
	struct QueueFamilyindices {
		std::optional<uint32_t> graphicFamily;
		std::optional<uint32_t> presentFamily;
		std::optional<uint32_t> transferFamily;
		std::optional<uint32_t> computeFamily;
		bool isComplete()
		{
			return graphicFamily.has_value() && presentFamily.has_value() && 
				transferFamily.has_value() && computeFamily.has_value();
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
	 VkShaderModule createShaderModule(const char* code, size_t len);
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
	 void create_descriptor_set_layout();
	 void populate_descriptor_set();
	 Vulkan_texture createTextureImage(mi::Size textureIdx);
	 void createTextureImageView();
	 VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspFlags,
		 uint32_t mipLevels);
	 void createImage(uint32_t width, uint32_t height, mi::Size texIdx, uint32_t layers, uint32_t mipLevels,
		 VkSampleCountFlagBits numSamples, VkFormat format,
		 VkImageTiling tiling, VkImageUsageFlags usage,
		 VkMemoryPropertyFlags properties, VkImage& image,
		 VkDeviceMemory& imageMemory);
	 void createColorResources();
	 VkCommandBuffer beginSingletimeCommands();
	 void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	 void transitionImageLayout(VkImage image, VkFormat format,
		 VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	 void generateMipmaps(VkImage image, int32_t texWidth,
		 int32_t texHeight, VkFormat imageFormat, uint32_t mipLevels);
	 void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t
		 width, uint32_t height , uint32_t layers = 1);
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
	 VkSampleCountFlagBits getMaxUsableSampleCount();
	 void create_ro_data_buffer();
	 void createtextureImages();
	 void create_path_trace_shader_module();
	 void create_material_textures_index_buffer(const std::vector<uint32_t>& indices);
	 void update(float elapsed_seconds, uint32_t image_index);
	 void prepareCompute();
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
	mi::base::Handle<mi::neuraylib::ITransaction> m_transaction;
	mi::base::Handle<const mi::neuraylib::ITarget_code> m_target_code;
	mi::base::Handle<mi::neuraylib::IMdl_impexp_api> m_mdl_impexp_api;
	mi::base::Handle<mi::neuraylib::IImage_api> m_image_api;

	std::vector<vertexIn> vertices;
	std::vector<uint32_t> Indices;
	struct User_data
	{
		uint32_t material_pattern;
		float animation_time;
	} m_user_data;

	struct uniformObject {
		glm::mat4 Model;
		glm::mat4 View;
		glm::mat4 Proj;
	} ;


	struct GLFWwindow* window;
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
	VkQueue  computeQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkExtent2D swapchainExtend;
	VkFormat swapchainFormat;
	std::vector<VkImageView> swapChainImageView;
	VkDescriptorSetLayout descriptorSetLayout;
	//VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
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

	VkBuffer mdlBuffer;
	VkDeviceMemory mdlBufferMemory;
	std::vector<Vulkan_texture> m_material_textures_2d;
	std::vector<Vulkan_texture> m_material_textures_3d;
	Vulkan_buffer materialIndexBuff;

	VkImage textureImage;
	VkImageView textureImageView;
	VkDeviceMemory textureImageMemory;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
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
	std::string   HlslSourceCode;
	
	std::vector< mi::lz::mdl::Shader_library> dxil_compiled_libraries;
	std::vector< mi::lz::mdl::Shader_library> vertexLibs;
	//std::vector<mi::lz::mdl::Shader_library> raygenProgLibs;
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

inline void exit_failure_(
	const char* file, int line,
	std::string message)
{
	// print message
	if (message.empty())
		fprintf(stderr, "Fatal error in file: %s line: %d\n\nClosing the example.\n", file, line);
	else
		fprintf(stderr, "Fatal error in file: %s line: %d\n  %s\n\nClosing the example.\n",
			file, line, message.c_str());

	// keep console open
#ifdef _WIN32
	if (IsDebuggerPresent()) {
		fprintf(stderr, "Press enter to continue . . . \n");
		fgetc(stdin);
	}
#endif

	// kill the application
	exit(EXIT_FAILURE);
}

inline void exit_failure_(const char* file, int line)
{
	exit_failure_(file, line, "");
}

#define exit_failure(...) \
    exit_failure_(__FILE__, __LINE__, mi::lz::strings::format(__VA_ARGS__))

#define check_success( expr) \
    do { \
        if( !(expr)) \
            exit_failure( "%s", #expr); \
    } while( false)


