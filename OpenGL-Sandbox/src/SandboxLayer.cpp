#include "SandboxLayer.h"
#define PI 3.1415926
#define g 9.8
#define G 6.67


using namespace GLCore;
using namespace GLCore::Utils;


SandboxLayer::SandboxLayer()
    : m_CameraController(16.0f / 9.0f), mousePressing(false), bound(glm::vec3(4.0f, 4.0f, 4.0f)), loss(0.997), external_force(0.0f), black_hole(glm::vec3(0.0f)), hole_weight(10.0f)
{
    Application& app = Application::Get();
    ratio = (float)app.GetWindow().GetWidth() / (float)app.GetWindow().GetHeight();
}

SandboxLayer::~SandboxLayer()
{
}

void SandboxLayer::OnAttach()
{
	EnableGLDebugging();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_Shader = Shader::FromGLSLTextFiles(
		"assets/shaders/ball.vert.glsl",
		"assets/shaders/ball.frag.glsl"
	);
	
    sky_Shader = Shader::FromGLSLTextFiles(
        "assets/shaders/skybox.vert.glsl",
        "assets/shaders/skybox.frag.glsl"
    );

    prepareSkybox();

	glCreateVertexArrays(1, &m_QuadVA);
	glBindVertexArray(m_QuadVA);
    const float r = 0.1f;

    for (int i = 0; i < 100; i++) {
        Ball* temp = new Ball();
        temp->position = glm::vec3((float)rand() / RAND_MAX * bound.x * 2 - bound.x, (float)rand() / RAND_MAX * bound.y * 2 - bound.y, (float)rand() / RAND_MAX * bound.z * 2 - bound.z);
        temp->velocity = glm::normalize(glm::cross(glm::normalize(temp->position - black_hole), glm::vec3(0.0f, 0.0f, 1.0f))) * glm::sqrt((float)G * hole_weight / glm::distance(temp->position, black_hole));
        temp->radius = r;
        balls.push_back(temp);
    }

    const int stacks = 10, sectors = 2 * stacks;
    

    for (int j = 0; j <= stacks; j++) {
        float v = (float)j / stacks;
        float phi = PI * v;

        for (int i = 0; i <= sectors; i++) {
            float u = (float)i / sectors;
            float theta = 2 * PI * u;

            VB.push_back(r * sin(phi) * cos(theta));
            VB.push_back(r * sin(phi) * sin(theta));
            VB.push_back(r * cos(phi));
            VB.push_back(sin(phi) * cos(theta));
            VB.push_back(sin(phi) * sin(theta));
            VB.push_back(cos(phi));
            texCoords.push_back(u); texCoords.push_back(v);
        }
    }
    for (int j = 0; j < stacks; j++) {
        int k1 = j * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int i = 0; i < sectors; i++, k1++, k2++) {
            if (j != 0) {
                indices.push_back(k1);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
            }

            if (j != stacks - 1) {
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                indices.push_back(k2 + 1);
            }
        }
    }

    glCreateBuffers(1, &m_QuadVB);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * VB.size(), &VB[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glCreateBuffers(1, &m_QuadIB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_QuadIB);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
}

void SandboxLayer::OnDetach()
{
    glDeleteVertexArrays(1, &m_QuadVA);
    glDeleteBuffers(1, &m_QuadVB);
    glDeleteBuffers(1, &m_QuadIB);
    glDeleteVertexArrays(1, &sky_QuadVA);
    glDeleteBuffers(1, &sky_QuadVB);
}

void SandboxLayer::OnEvent(Event& event)
{
    m_CameraController.OnEvent(event);

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        [&](MouseButtonPressedEvent& e)
        {
            for (auto ball : balls) {
                ball->force += glm::normalize(-ball->position) * (float)g * ball->weight * external_force;
            }
            //m_SquareColor = m_SquareAlternateColor;
            mousePressing = true;
            firstMouse = true;
            return false;
        });
    dispatcher.Dispatch<MouseButtonReleasedEvent>(
        [&](MouseButtonReleasedEvent& e)
        {
            //m_SquareColor = m_SquareBaseColor;
            mousePressing = false;
            firstMouse = false;
            return false;
        });
    dispatcher.Dispatch<MouseMovedEvent>(
        [&](MouseMovedEvent& e)
        {
            if (mousePressing) {
                if (firstMouse) {
                    lastX = e.GetX();
                    lastY = e.GetY();
                    firstMouse = false;
                }
                float xOffset = e.GetX() - lastX;
                float yOffset = lastY - e.GetY();

                lastX = e.GetX();
                lastY = e.GetY();
                m_ProjectionCamera.ProcessMouseMovement(xOffset, yOffset);
            }
            return false;
        });
    dispatcher.Dispatch<MouseScrolledEvent>(
        [&](MouseScrolledEvent& e)
        {
            m_ProjectionCamera.ProcessMouseScroll(e.GetYOffset());
            return false;
        });
}

void SandboxLayer::OnUpdate(Timestep ts)
{
    m_ProjectionCamera.OnUpdate(ts);
    //OnUpdatePhysics(ts);
    OnUpdateSolar(ts);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 projection = glm::perspective(glm::radians(m_ProjectionCamera.Zoom), ratio, 0.1f, 100.0f);
    glm::mat4 view = m_ProjectionCamera.GetViewMatrix();

    // skybox render
    glDepthMask(GL_FALSE);
    glUseProgram(sky_Shader->GetRendererID());
    glBindVertexArray(sky_QuadVA);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sky_Texture);
    int location = glGetUniformLocation(sky_Shader->GetRendererID(), "projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projection));
    location = glGetUniformLocation(sky_Shader->GetRendererID(), "view");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    // Objects render
    glUseProgram(m_Shader->GetRendererID());

    location = glGetUniformLocation(m_Shader->GetRendererID(), "projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projection));

    location = glGetUniformLocation(m_Shader->GetRendererID(), "view");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view));

    location = glGetUniformLocation(m_Shader->GetRendererID(), "u_Color");
    glUniform4fv(location, 1, glm::value_ptr(m_SquareColor));

    for (auto ball : balls) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, ball->position);
        location = glGetUniformLocation(m_Shader->GetRendererID(), "model");
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(m_QuadVA);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    }
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(3.0f));
    model = glm::translate(model, black_hole);
    location = glGetUniformLocation(m_Shader->GetRendererID(), "model");
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(m_QuadVA);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
}

void SandboxLayer::OnImGuiRender()
{
    ImGui::Begin("Controls");
    ImGui::SliderFloat3("black hole", glm::value_ptr(black_hole), 0.0f, 4.0f);
    ImGui::SliderFloat("black hole weight", &hole_weight, 3.0f, 100.0f);
    ImGui::SliderFloat3("bound box", glm::value_ptr(bound),0.0f, 8.0f);
    ImGui::SliderFloat("loss", &loss, 0.8f, 1.0f);
    ImGui::SliderFloat("external force", &external_force, 0.0f, 300.0f);
    ImGui::End();
}

void SandboxLayer::OnUpdatePhysics(GLCore::Timestep ts)
{
    for (auto ball : balls) {
        ball->position += ball->velocity * ts.GetSeconds();
        if (ball->position.y - ball->radius < -bound.y || ball->position.y + ball->radius > bound.y) {
            if (ball->position.y - ball->radius < -bound.y) {
                ball->position.y = -bound.y + ball->radius;
            }
            else {
                ball->position.y = bound.y - ball->radius;
            }
            ball->acceleration.y = -ball->acceleration.y;
            ball->velocity.y = -ball->velocity.y;
        }
        if (ball->position.x - ball->radius < -bound.x || ball->position.x + ball->radius > bound.x) {
            if (ball->position.x - ball->radius < -bound.x) {
                ball->position.x = -bound.x + ball->radius;
            }
            else {
                ball->position.x = bound.x - ball->radius;
            }
            ball->acceleration.x = -ball->acceleration.x;
            ball->velocity.x = -ball->velocity.x;
        }
        if (ball->position.z - ball->radius < -bound.z || ball->position.z + ball->radius > bound.z) {
            if (ball->position.z - ball->radius < -bound.z) {
                ball->position.z = ball->radius - bound.z;
            }
            else {
                ball->position.z = -ball->radius + bound.z;
            }
            ball->acceleration.z = -ball->acceleration.z;
            ball->velocity.z = -ball->velocity.z;
        }
    }
    for (int i = 0; i < balls.size(); i++) {
        for (int j = i + 1; j < balls.size(); j++) {
            float dis = glm::distance(balls[i]->position, balls[j]->position);
            if (dis < balls[i]->radius + balls[j]->radius) {
                float error = balls[i]->radius + balls[j]->radius - dis;
                glm::vec3 direction = glm::normalize(balls[i]->position - balls[j]->position);
                glm::mat4 rotationMat(1.0f);
                rotationMat = glm::rotate(rotationMat, glm::radians(180.0f), direction);

                balls[i]->position += error / 2.0f * direction;
                balls[j]->position += -error / 2.0f * direction;

                balls[i]->velocity = -glm::vec3(rotationMat * glm::vec4(balls[i]->velocity, 1.0f));
                balls[i]->acceleration = -glm::vec3(rotationMat * glm::vec4(balls[i]->acceleration, 1.0f));
                balls[j]->velocity = -glm::vec3(rotationMat * glm::vec4(balls[j]->velocity, 1.0f));
                balls[j]->acceleration = -glm::vec3(rotationMat * glm::vec4(balls[j]->acceleration, 1.0f));
            }
        }
    }
    for (auto ball : balls) {
        ball->force += glm::vec3(0, -g * ball->weight, 0);
        ball->acceleration = ball->force / ball->weight;
        ball->velocity += ball->acceleration * ts.GetSeconds();
        ball->velocity *= loss;
        ball->force = glm::vec3(0.0f);
    }
}

void SandboxLayer::OnUpdateSolar(GLCore::Timestep ts)
{
    for (auto ball : balls) {
        ball->position += ball->velocity * ts.GetSeconds();

        float distance = glm::distance(ball->position, black_hole);
        glm::vec3 direction = -glm::normalize(ball->position - black_hole);
        if (distance < 2.0f) {
            distance = 2.0f;
        }
        ball->force += direction * (float)G * ball->weight * hole_weight / (distance * distance);
        ball->acceleration = ball->force / ball->weight;
        ball->velocity += ball->acceleration * ts.GetSeconds();
        ball->force = glm::vec3(0.0f);
    }
}

void SandboxLayer::OnUpdatemultibody(GLCore::Timestep ts)
{
}

void SandboxLayer::loadCubemap(std::vector<std::string> faces) {
    glGenTextures(1, &sky_Texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, sky_Texture);
    int width, height, nrComponents;

    for (int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            LOG_ERROR("Cubemap texture failed to load at path:", faces[i]);
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void SandboxLayer::prepareSkybox()
{
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &sky_QuadVA);
    glGenBuffers(1, &sky_QuadVB);
    glBindVertexArray(sky_QuadVA);
    glBindBuffer(GL_ARRAY_BUFFER, sky_QuadVB);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces
    {
        "assets/textures/skybox/right.jpg",
        "assets/textures/skybox/left.jpg",
        "assets/textures/skybox/top.jpg",
        "assets/textures/skybox/bottom.jpg",
        "assets/textures/skybox/front.jpg",
        "assets/textures/skybox/back.jpg",
    };
    loadCubemap(faces);
}
