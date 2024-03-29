#ifndef VULKAN_HANDLER
#define VULKAN_HANDLER

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <set>
#include <cstdint>
#include <algorithm>

#include "QueueFamilyIndices.h"
#include "SwapChainSupportDetails.h"

class VulkanHandler {
public:
	GLFWwindow* initWindow();
	void initVulkan();
	void cleanup();
	void recreateSwapChainVulkan();
	void cleanupSwapChainVulkan();

	VkDevice& getDevice();
	VkExtent2D& getSwapchainExtent();
	VkFormat& getSwapchainFormat();
	std::vector<VkImageView>& getSwapChainImageViews();
	std::vector<VkImage>& getSwapChainImages();
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	VkPhysicalDevice& getPhysicalDevice();
	VkSwapchainKHR& getSwapChain();
	VkQueue& getGraphicsQueue();
	VkQueue& getPresentQueue();
	bool getFramebufferResized();
	void setFramebufferResized(bool resized);

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
	void createInstance();
	bool checkValidationLayerSupport();
	void pickPhysicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	void createLogicalDevice();
	void createSurface();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createSwapChain();
	void createImageViews();

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkSurfaceKHR surface;
	VkQueue presentQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	bool framebufferResized = false;

};

#endif