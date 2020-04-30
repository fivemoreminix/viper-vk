#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cstdint>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class TriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
		createGraphicsPipeline();
	}

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	// Swap chain
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;

	// Render pipeline
	VkPipelineLayout m_pipelineLayout;

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW not to load OpenGL (as it does by default)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Prevent resizing windows, as it requires special care

		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Triangle", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
	}

	/// Create the Vulkan instance.
	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &app_info;

		uint32_t glfw_extension_count = 0;
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		createInfo.enabledExtensionCount = glfw_extension_count;
		createInfo.ppEnabledExtensionNames = glfw_extensions;

		if (enableValidationLayers) { // Basically a runtime safety feature ...
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()); // Retrieve a list of supported extensions

		std::cout << "available extensions:\n";
		for (const auto& extension : extensions) {
			std::cout << '\t' << extension.extensionName << '\n';
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Vulkan m_instance!");
		}
	}

	void createSurface() {
		if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	bool checkValidationLayerSupport() {
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validationLayers) {
			bool layer_found = false;

			for (const auto& layerProperties : available_layers) {
				if (strcmp(layer_name, layerProperties.layerName) == 0) {
					layer_found = true;
					break;
				}
			}

			if (!layer_found) return false;
		}

		return true;
	}

	/// Select a GPU with Vulkan support to use for rendering.
	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

		for (const auto& dev : devices) {
			if (isDeviceSuitable(dev)) {
				m_physicalDevice = dev;
				break;
			}
		}

		if (m_physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	/// Returns true if the given GPU has valid queue families and supports required device extensions and features for the swap chain.
	bool isDeviceSuitable(VkPhysicalDevice dev) {
		QueueFamilyIndices indices = findQueueFamilies(dev);

		bool extensionsSupported = checkDeviceExtensionSupport(dev);

		bool swapChainAdequate = false;
		if (extensionsSupported) { // If the swap chain extension is supported ...
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return indices.isComplete() && extensionsSupported && swapChainAdequate;
	}

	/// Returns true if the given GPU supports the required extensions found in `requiredDeviceExtensions`.
	bool checkDeviceExtensionSupport(VkPhysicalDevice dev) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	/// Returns the queue families for graphics and rendering in a QueueFamilyIndices struct.
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev) {
		QueueFamilyIndices indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families.data());

		int i = 0;
		for (const auto& queue_family : queue_families) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentSupport); // Does this GPU support rendering to surfaces for this queue family?

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				break;
			}

			i++;
		}

		return indices;
	}

	/// Create the logical device `m_device` to the physical GPU `m_physicalDevice`.
	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		// Turn queue family indexes into queue family creation infos ...
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		const float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		VkPhysicalDeviceFeatures device_features{}; // Use default values

		// Now we complete the information about the logical device we're creating ...
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pQueueCreateInfos = queue_create_infos.data();

		// Enable device extensions
		create_info.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
		create_info.ppEnabledExtensionNames = requiredDeviceExtensions.data();

		create_info.pEnabledFeatures = &device_features;

		if (enableValidationLayers) { // Keeping support for older implementations of Vulkan
			create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			create_info.ppEnabledLayerNames = validationLayers.data();
		} else {
			create_info.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_physicalDevice, &create_info, nullptr, &m_device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_presentQueue);
	}

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/// Get capabilities, formats, and present mode support from the device for a swap chain.
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev) {
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, m_surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(dev, m_surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(dev, m_surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	/// Select best available swap chain surface format.
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& available_format : availableFormats) {
			// We would always prefer 32-bit SRGB pixels, in the SRGB non-linear color space if it is available.
			if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return available_format;
			}
		}

		return availableFormats[0]; // Choose first format available.
	}

	/// Select best swap chain method for displaying images from the queue.
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& available_present_mode : availablePresentModes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) { // Choose triple-buffering when it is available
				return available_present_mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR; // Double-buffering is always available. (Commonly known as Vsync in video games)
	}

	/// Select resolution of the swap chain images.
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		} else {
			VkExtent2D actual_extent = {WIDTH, HEIGHT};

			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actual_extent;
		}
	}

	void createSwapChain() {
		SwapChainSupportDetails swap_chain_support = querySwapChainSupport(m_physicalDevice);

		// Of the swap chain's supported modes and formats, we will choose the best options for the three:
		VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
		VkPresentModeKHR present_mode = chooseSwapPresentMode(swap_chain_support.presentModes);
		VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);

		// Assign some member variables ...
		m_swapChainImageFormat = surface_format.format;
		m_swapChainExtent = extent;

		// Request at least one more than the minimum images for the swap chain
		uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

		// Make sure we don't exceed the maximum, either ...
		if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
			image_count = swap_chain_support.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = m_surface;

		// Details of the swap chain
		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1;
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		// The swap chain needs to know which queue families will own the images ...
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
		uint32_t queue_family_indices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		if (indices.graphicsFamily != indices.presentFamily) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // No ownership promises (simpler)
			create_info.queueFamilyIndexCount = 2; // How many queue families will share the images?
			create_info.pQueueFamilyIndices = queue_family_indices; // Which queue families will share them?
		} else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Explicit ownership (higher performance)
			create_info.queueFamilyIndexCount = 0; // Optional
			create_info.pQueueFamilyIndices = nullptr; // Optional
		}

		// Some miscellaneous swap chain creation information.
		create_info.preTransform = swap_chain_support.capabilities.currentTransform;
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = present_mode;
		create_info.clipped = VK_TRUE;
		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create the swap chain!");
		}

		// Obtain references to the swap chain images
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &image_count, nullptr);
		m_swapChainImages.resize(image_count);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &image_count, m_swapChainImages.data());
	}

	void createImageViews() {
		m_swapChainImageViews.resize(m_swapChainImages.size());

		for (size_t i = 0; i < m_swapChainImages.size(); i++) {
			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = m_swapChainImages[i];

			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; // How do we treat images?
			create_info.format = m_swapChainImageFormat;

			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// What is the image's purpose, and which part of the image should be accessed?
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_device, &create_info, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	/// Read a file from the file system given a filename, as a vector of bytes.
	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary); // Read at the end (ate) in binary

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		size_t file_size = (size_t) file.tellg(); // Get file size using position currently at (end)
		std::vector<char> buffer(file_size); // Resize our buffer
		file.seekg(0); // Go to the beginning of the file
		file.read(buffer.data(), file_size); // Read the data

		file.close();
		return buffer;
	}

	/// Construct a new Vulkan shader module from a byte vector.
	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
		
		VkShaderModule shader_module;
		if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return shader_module;
	}

	void createGraphicsPipeline() {
		auto vert_shader_code = readFile("../shaders/vert.spv");
		auto frag_shader_code = readFile("../shaders/frag.spv");

		VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
		VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{}; // Vertex shader module info
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module;
		vert_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_shader_stage_info{}; // Frag shader module info
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vert_shader_stage_info, frag_shader_stage_info};

		// Describes the format of the vertex data that will be passed to the vertex shader
		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = nullptr; // Optional
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = nullptr; // Optional

		// What kind of geometry should be drawn from vertices
		VkPipelineInputAssemblyStateCreateInfo input_asm{};
		input_asm.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_asm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // For the type of primitive restart
		input_asm.primitiveRestartEnable = VK_FALSE; // Disabled because we only draw triangles

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) m_swapChainExtent.width;
		viewport.height = (float) m_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // If true, it discards any output from reaching frame buffer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Determines how fragments are generated for geometry
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Type of culling
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// TODO: multisampling
		/*
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional
		*/

		// TODO: depth buffering

		// Alpha blending
		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f; // Optional
		color_blending.blendConstants[1] = 0.0f; // Optional
		color_blending.blendConstants[2] = 0.0f; // Optional
		color_blending.blendConstants[3] = 0.0f; // Optional

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0; // Optional
		pipeline_layout_info.pSetLayouts = nullptr; // Optional
		pipeline_layout_info.pushConstantRangeCount = 0; // Optional
		pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		vkDestroyShaderModule(m_device, frag_shader_module, nullptr);
		vkDestroyShaderModule(m_device, vert_shader_module, nullptr);
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		// Vulkan cleanup
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		for (auto image_view : m_swapChainImageViews) { // Destroy image views
			vkDestroyImageView(m_device, image_view, nullptr);
		} // (Images are destroyed automatically by destroying the swap chain)
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
		vkDestroyDevice(m_device, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);

		// GLFW cleanup
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}
};

int main() {
	TriangleApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
