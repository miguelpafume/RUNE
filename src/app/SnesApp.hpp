#pragma once

#include "Camera.hpp"
#include "Engine.hpp"
#include "GameObject.hpp"

#include "Texture.hpp"

#include "snes/System.hpp"

#include <string>

class SnesApp {
public:
	explicit SnesApp(const std::string& romPath);

	void run();

private:
	void loadPlane();
	void updateAspect(float aspect);
	void pollInput();

	MAGE::Engine m_engine;
	SNES::System m_snes;

	std::vector<MAGE::GameObject>  m_gameObjects;
	std::unique_ptr<MAGE::Texture> m_displayTexture;

	MAGE::Camera m_camera {};
	float        m_top {}, m_bottom {}, m_left {}, m_right {};
};