#include "GraphicsEngine.h"

#include "glm/ext.hpp" //Delete THis

const std::string MODEL_PATH = "models/round.obj";
const std::string TEXTURE_PATH = "textures/Gray.jpg";

const int RENDERED_OBJECTS = 12;

const int MAX_FRAMES_IN_FLIGHT = 2;

void GraphicsEngine::init() {
	vh = VulkanHandler();
	window = vh.initWindow();
	vh.initVulkan();
	gph = GraphicsPipelineHandler(&vh);
	ol = ObjLoader();
	ol.loadModel(MODEL_PATH);
	gph.setTexturePath(TEXTURE_PATH);
	gph.setVertices(ol.getVertices());
	gph.setIndices(ol.getIndices());
	gph.createRenderedObjects(RENDERED_OBJECTS);
	gph.initGraphicsPipeline();
	createSyncObjects();
	createFigure();
	pe = PhysicsEngine(glm::vec3(0.0f, 1.0f, 0.0f), 2.0f);
	mainLoop();
	cleanup();
}

void GraphicsEngine::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(vh.getDevice());
}

void GraphicsEngine::drawFrame() {
	vkWaitForFences(vh.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(vh.getDevice(), vh.getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Unable to acquire image for the swap chain.");
	}

	updateUniformBuffer(imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &(gph.getCommandBuffers())[imageIndex];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(vh.getDevice(), 1, &inFlightFences[currentFrame]);

	if (vkQueueSubmit(vh.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("Unable to submit draw command.");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { vh.getSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(vh.getPresentQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vh.getFramebufferResized()) {
		vh.setFramebufferResized(false);
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("Unable to present image from swapchain.");
	}

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void GraphicsEngine::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		if (vkCreateSemaphore(vh.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(vh.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(vh.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create sync objects.");
		}
	}
}

void GraphicsEngine::recreateSwapChain() {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vh.getDevice());

	cleanupSwapChain();

	vh.recreateSwapChainVulkan();
	gph.recreateSwapChainGraphicsPipeline();
}

void GraphicsEngine::updateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	startTime = currentTime;

	//Create Figure
	nodeIterator = 0;
	figureUpdate(root, currentImage, time);
	pe.applyAcceleration(root);
	//End Create Figure
	/*
	for (int i = 0; i < gph.getRenderedObjectsSize(); i++) {
		UniformBufferObject ubo = {};

		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, -1.0f, .0f));
		ubo.model = glm::scale(ubo.model, glm::vec3(0.5f, 0.5f, 0.5f));

		if (i == 0) {
			ubo.model = glm::translate(ubo.model, glm::vec3(1.0f * time, .0f, .0f));
		}
		else {
			ubo.model = glm::translate(ubo.model, glm::vec3(-1.0f * time, .0f, .0f));
		}

		ubo.view = glm::lookAt(glm::vec3(0.0f, 1.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vh.getSwapchainExtent().width / (float)vh.getSwapchainExtent().height, 0.1f, 10.0f);

		ubo.proj[1][1] *= -1;

		void* data;
		vkMapMemory(vh.getDevice(), gph.getUniformBuffersMemory()[currentImage + (i * vh.getSwapChainImages().size())], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(vh.getDevice(), gph.getUniformBuffersMemory()[currentImage + (i * vh.getSwapChainImages().size())]);
	}
	*/
}

void GraphicsEngine::cleanupSwapChain() {
	gph.cleanupSwapchainGraphicsPipeline();
	vh.cleanupSwapChainVulkan();
}

void GraphicsEngine::cleanupFigureNodes(FigureNodes* node) {
	for (auto child : node->children) {
		cleanupFigureNodes(child);
	}
	delete node;
}

void GraphicsEngine::cleanup() {
	cleanupFigureNodes(root);
	cleanupSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(vh.getDevice(), renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(vh.getDevice(), imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(vh.getDevice(), inFlightFences[i], nullptr);
	}

	gph.cleanup();
	vh.cleanup();
}

void GraphicsEngine::createFigure() {
	root = new FigureNodes();
	root->pos = glm::vec3(0.0f, 0.0f, 0.0f);
	FigureNodes* neck = new FigureNodes();
	neck->parent = root;
	neck->pos = glm::vec3(-sqrt(.5), -sqrt(.5), 0.0);
	FigureNodes* head = new FigureNodes();;
	head->parent = neck;
	head->pos = glm::vec3(-1.0 - sqrt(.5), -sqrt(.5), 0.0);
	neck->children.push_back(head);
	
	FigureNodes* rufleg = new FigureNodes();
	rufleg->parent = root;
	rufleg->pos = glm::vec3(0.0 - .924, .383, 0.2);
	FigureNodes* rlfleg = new FigureNodes();
	rlfleg->parent = rufleg;
	rlfleg->pos = glm::vec3(0.0 - .924, 1.383, 0.2);
	rlfleg->gravity = true;
	rufleg->children.push_back(rlfleg);

	FigureNodes* lufleg = new FigureNodes();
	lufleg->parent = root;
	lufleg->pos = glm::vec3(0.0 - .383, .924, -0.2);
	FigureNodes* llfleg = new FigureNodes();
	llfleg->parent = lufleg;
	llfleg->pos = glm::vec3(0.0 - .383, 1.924, -0.2);
	llfleg->vel.y = -0.783f;
	llfleg->gravity = true;
	lufleg->children.push_back(llfleg);

	FigureNodes* midBack = new FigureNodes();
	midBack->parent = root;
	midBack->pos = glm::vec3(1.0, 0.0, 0.0);

	FigureNodes* back = new FigureNodes();
	back->parent = midBack;
	back->pos = glm::vec3(2.0, 0.0, 0.0);

	FigureNodes* rubleg = new FigureNodes();
	rubleg->parent = back;
	rubleg->pos = glm::vec3(2.0, 1.0, 0.2);
	FigureNodes* rlbleg = new FigureNodes();
	rlbleg->parent = rubleg;
	rlbleg->pos = glm::vec3(2.0, 2.0, 0.2);
	rlbleg->vel.y = -1.175f;
	rlbleg->gravity = true;
	rubleg->children.push_back(rlbleg);
	
	FigureNodes* lubleg = new FigureNodes();
	lubleg->parent = back;
	lubleg->pos = glm::vec3(2.0 - sqrt(.5), sqrt(.5), -0.2);
	FigureNodes* llbleg = new FigureNodes();
	llbleg->parent = lubleg;
	llbleg->pos = glm::vec3(2.0 - sqrt(.5), 1.0 + sqrt(.5), -0.2);
	llbleg->vel.y = -0.392f;
	llbleg->gravity = true;
	lubleg->children.push_back(llbleg);

	back->children.push_back(rubleg);
	back->children.push_back(lubleg);

	midBack->children.push_back(back);

	root->children.push_back(neck);
	root->children.push_back(rufleg);
	root->children.push_back(lufleg);
	root->children.push_back(midBack);
}

void GraphicsEngine::figureUpdate(FigureNodes* node, uint32_t currentImage, float time) {
	pe.applyPhysics(node, time);

	if (node->parent != NULL) {
		UniformBufferObject ubo = {};

		ubo.model = glm::mat4(1.0f);
		
		ubo.model = glm::translate(ubo.model, (node->pos + node->parent->pos) / 2.0f);

		if (node->parent->pos.y >= node->pos.y) {
			ubo.model = glm::rotate(ubo.model, glm::acos(glm::dot(glm::normalize(node->parent->pos - node->pos), glm::vec3(1.0f, 0.0f, 0.0f))), glm::vec3(0.0f, 0.0f, 1.0f));
		}
		else {
			ubo.model = glm::rotate(ubo.model, -glm::acos(glm::dot(glm::normalize(node->parent->pos - node->pos), glm::vec3(1.0f, 0.0f, 0.0f))), glm::vec3(0.0f, 0.0f, 1.0f));
		}
		//ubo.model = glm::rotate(ubo.model, glm::acos(glm::dot(glm::normalize(node.parent->pos - node.pos), glm::vec3(1.0f, 0.0f, 0.0f))), glm::vec3(0.0f, 0.0f, 1.0f));
		//ubo.model = glm::rotate(ubo.model, glm::acos(glm::dot(glm::normalize(node.parent->pos - node.pos), glm::vec3(1.0f, 0.0f, 0.0f))), glm::vec3(0.0f, 0.0f, 1.0f));

		ubo.model = glm::scale(ubo.model, glm::vec3(1.0f, 0.3f, 0.3f));
	
		ubo.view = glm::lookAt(glm::vec3(-3.0f, -.2f, 6.0f), glm::vec3(-3.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vh.getSwapchainExtent().width / (float)vh.getSwapchainExtent().height, 0.1f, 30.0f);

		void* data;
		vkMapMemory(vh.getDevice(), gph.getUniformBuffersMemory()[currentImage + (nodeIterator * vh.getSwapChainImages().size())], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(vh.getDevice(), gph.getUniformBuffersMemory()[currentImage + (nodeIterator * vh.getSwapChainImages().size())]);

		nodeIterator++;
	}

	for (auto child : node->children) {
		figureUpdate(child, currentImage, time);
	}
}
