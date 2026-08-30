#pragma once

#include "Engine.hpp"
#include "Buffer.hpp"
#include "Camera.hpp"
#include "GameObject.hpp"
#include "KeyboardController.hpp"
#include "Model.hpp"
#include "RenderSystem.hpp"
#include "Texture.hpp"
#include "Util.hpp"

#include "Types.hpp"
#include "Snes.hpp"

#include <string>

class SnesApp {
public:
    explicit SnesApp(const std::string& romPath);
    ~SnesApp() = default;

    void run();

private:
    void loadPlane();
    void updateAspect(float aspect);
    void pollInput();

    MAGE::Engine m_engine;
    RUNE::Snes m_snes;

    std::vector<MAGE::GameObject> m_gameObjects;
    std::unique_ptr<MAGE::Texture> m_displayTexture;

    MAGE::Camera m_camera {};
    float m_top {}, m_bottom {}, m_left {}, m_right {};
};