#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>
#include <vector>
#include <stb_image/stb_image.h>
#include "Camera.h"
#include "Ball.h"

class SandboxLayer : public GLCore::Layer
{
public:
	SandboxLayer();
	virtual ~SandboxLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnUpdatePhysics(GLCore::Timestep ts);
	void OnUpdateSolar(GLCore::Timestep ts);
	void OnUpdatemultibody(GLCore::Timestep ts);
	void prepareSkybox();
	void loadCubemap(std::vector<std::string> faces);

	float ratio;
private:
	GLCore::Utils::Shader* m_Shader;
	GLCore::Utils::Shader* sky_Shader;
	GLCore::Utils::OrthographicCameraController m_CameraController;
	Camera m_ProjectionCamera;

	GLuint m_QuadVA, m_QuadVB, m_QuadIB;
	GLuint sky_QuadVA, sky_QuadVB;
	GLuint sky_Texture;

	Ball ball;
	std::vector<Ball*> balls;

	glm::vec4 m_SquareBaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 m_SquareAlternateColor = { 0.2f, 0.3f, 0.8f, 1.0f };
	glm::vec4 m_SquareColor = m_SquareBaseColor;

	std::vector<float> VB, texCoords;
	std::vector<int> indices;

	bool mousePressing;
	bool firstMouse;
	float lastX, lastY;
	glm::vec3 bound;
	float loss;
	float external_force;
	
	glm::vec3 black_hole;
	float hole_weight;
};