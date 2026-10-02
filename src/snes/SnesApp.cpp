#include "SnesApp.hpp"

#include <chrono>
#include <iostream>
#include <utility>

SnesApp::SnesApp(const std::string& romPath) {
    loadPlane();

    m_snes.loadRom(romPath);
}

void SnesApp::run() {
    // UBO buffers
    std::vector<std::unique_ptr<MAGE::Buffer>> uboBuffers(MAGE::SwapChain::MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < uboBuffers.size(); i++) {
        uboBuffers[i] = std::make_unique<MAGE::Buffer>(
            m_engine.getDevice(),
            sizeof(MAGE::GlobalUbo),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_engine.getDevice().properties.limits.minUniformBufferOffsetAlignment);
        uboBuffers[i]->map();
    }

    // Global + texture descriptor set layouts
    auto globalSetLayout = MAGE::DescriptorSetLayout::Builder(m_engine.getDevice())
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .build();

    auto textureSetLayout = MAGE::DescriptorSetLayout::Builder(m_engine.getDevice())
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(MAGE::SwapChain::MAX_FRAMES_IN_FLIGHT);
    
    for (size_t i = 0; i < globalDescriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MAGE::DescriptorWriter(*globalSetLayout, *m_engine.getGlobalPool())
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    // The PPU framebuffer is uploaded once per frame with nearest-neighbor filtering
    m_displayTexture = std::make_unique<MAGE::Texture>(m_engine.getDevice(), m_snes.framebuffer(), SNES::VIDEO_WIDTH, SNES::VIDEO_HEIGHT);
    
    VkDescriptorImageInfo displayInfo = m_displayTexture->descriptorInfo();
    VkDescriptorSet displayDescriptorSet;
    MAGE::DescriptorWriter(*textureSetLayout, *m_engine.getGlobalPool())
        .writeImage(0, &displayInfo)
        .build(displayDescriptorSet);
    m_gameObjects[0].m_textureDescriptorSet = displayDescriptorSet;

    MAGE::RenderSystem renderSystem{
        m_engine.getDevice(), m_engine.getRenderer().getSwapChainRenderPass(),
        globalSetLayout->getDescriptorSetLayout(), textureSetLayout->getDescriptorSetLayout()};

    float aspect = m_engine.getRenderer().getAspectRatio();
    updateAspect(aspect);

    auto currentTime = std::chrono::high_resolution_clock::now();

    while (!m_engine.getWindow().shouldClose()) {
        glfwPollEvents();

        auto newTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        dt = std::min(dt, 0.1f);
        currentTime = newTime;

        if (aspect != m_engine.getRenderer().getAspectRatio()) {
            aspect = m_engine.getRenderer().getAspectRatio();
            updateAspect(aspect);
        }

        pollInput();

        m_snes.runFrame();
        m_displayTexture->update(m_snes.framebuffer());

        if (VkCommandBuffer cb = m_engine.getRenderer().beginFrame()) {
            const int frameIndex = m_engine.getRenderer().getFrameIndex();
            MAGE::FrameInfo frame{frameIndex, dt, cb, m_camera, globalDescriptorSets[frameIndex]};

            MAGE::GlobalUbo ubo{};
            ubo.projectionView = m_camera.getProjection() * m_camera.getView();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);

            m_engine.getRenderer().beginSwapChainRenderPass(cb);
            renderSystem.renderGameObject(frame, m_gameObjects);
            m_engine.getRenderer().endSwapChainRenderPass(cb);
            m_engine.getRenderer().endFrame();
        }
    }

    vkDeviceWaitIdle(m_engine.getDevice().getDevice());
}

void SnesApp::loadPlane() {
    MAGE::Builder planeBuilder {};
    planeBuilder.vertices = {
		{{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
		{{-1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
		{{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}
	};

	planeBuilder.indices = {0, 1, 2, 0, 2, 3};

	std::shared_ptr<MAGE::Model> planeModel = std::make_shared<MAGE::Model>(m_engine.getDevice(), planeBuilder);
	MAGE::GameObject plane = MAGE::GameObject::createGameObject();
	plane.m_model = planeModel;
	m_gameObjects.push_back(std::move(plane));
}

void SnesApp::updateAspect(float aspect) {
	if (aspect > SNES::DISPLAY_ASPECT) {
		m_left = -aspect / SNES::DISPLAY_ASPECT;
		m_right = aspect / SNES::DISPLAY_ASPECT;
		m_top = -1.0f;
		m_bottom = 1.0f;
	} else {
		m_left = -1.0f;
		m_right = 1.0f;
		m_top = -SNES::DISPLAY_ASPECT / aspect;
		m_bottom = SNES::DISPLAY_ASPECT / aspect;
	}

	m_camera.setOrtohraphicProjection(m_left, m_right, m_top, m_bottom, -1.0f, 1.0f);
}

void SnesApp::pollInput() {
    
}