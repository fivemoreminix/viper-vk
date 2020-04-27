#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <optional>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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
	}

private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;

	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW not to load OpenGL (as it does by default)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Prevent resizing windows, as it requires special care

		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Triangle", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		pickPhysicalDevice();
		createLogicalDevice();
	}

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

	bool isDeviceSuitable(VkPhysicalDevice dev) {
		QueueFamilyIndices indices = findQueueFamilies(dev);

		return indices.isComplete();
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() {
			return graphicsFamily.has_value();
		}
	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev) {
		QueueFamilyIndices indices;

		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families.data());

		int i = 0;
		for (const auto& queue_family : queue_families) {
			if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
				break;
			}

			i++;
		}

		return indices;
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphicsFamily.value();
		queue_create_info.queueCount = 1;

		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.queueCreateInfoCount = 1;

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
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(m_window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		// Vulkan cleanup
		vkDestroyDevice(m_device, nullptr);
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
