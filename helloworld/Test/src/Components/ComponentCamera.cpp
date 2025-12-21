#include "ComponentCamera.h"
#include "ComponentTransform.h"
#include "GameObject.h"
#include "imgui.h"
#include <glm/gtc/matrix_transform.hpp>

ComponentCamera::ComponentCamera(GameObject* owner)
    : Component(owner, ComponentType::CAMERA),
    fov(60.0f),
    nearPlane(0.1f),
    farPlane(1000.0f),
    aspectRatio(16.0f / 9.0f),
    projectionMode(ProjectionMode::PERSPECTIVE),
    orthoSize(10.0f),
    isMainCamera(false),
    backgroundColor(0.2f, 0.3f, 0.4f)
{
}

ComponentCamera::~ComponentCamera()
{
}

void ComponentCamera::Enable()
{
    // Called when component is enabled
}

void ComponentCamera::Update()
{
    // Update is called every frame when active
    // The camera uses the GameObject's transform for position/rotation
}

void ComponentCamera::Disable()
{
    // Called when component is disabled
}

glm::mat4 ComponentCamera::GetViewMatrix() const
{
    if (!owner)
        return glm::mat4(1.0f);

    ComponentTransform* transform = owner->GetComponent<ComponentTransform>();
    if (!transform)
        return glm::mat4(1.0f);

    // Get global transform
    glm::vec3 position = transform->GetGlobalPosition();
    glm::quat rotation = transform->GetGlobalRotation();

    // Calculate forward, right, up vectors from rotation
    glm::mat4 rotationMatrix = glm::mat4_cast(rotation);

    // Unity-like forward (negative Z in local space)
    glm::vec3 forward = -glm::vec3(rotationMatrix[2]);
    glm::vec3 up = glm::vec3(rotationMatrix[1]);

    // Create view matrix looking from position along forward direction
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 ComponentCamera::GetProjectionMatrix() const
{
    if (projectionMode == ProjectionMode::PERSPECTIVE)
    {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
    else
    {
        float halfWidth = orthoSize * aspectRatio;
        float halfHeight = orthoSize;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }
}

void ComponentCamera::OnEditor()
{
    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Main Camera", &isMainCamera);

        if (isMainCamera)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ACTIVE]");
        }

        ImGui::Separator();

        // Projection Mode
        const char* projModes[] = { "Perspective", "Orthographic" };
        int currentMode = (int)projectionMode;
        if (ImGui::Combo("Projection", &currentMode, projModes, 2))
        {
            projectionMode = (ProjectionMode)currentMode;
        }

        ImGui::Separator();

        // Perspective settings
        if (projectionMode == ProjectionMode::PERSPECTIVE)
        {
            ImGui::SliderFloat("Field of View", &fov, 10.0f, 120.0f);
        }
        else
        {
            ImGui::SliderFloat("Orthographic Size", &orthoSize, 0.1f, 100.0f);
        }

        // Common settings
        ImGui::DragFloat("Near Plane", &nearPlane, 0.01f, 0.01f, farPlane - 0.1f);
        ImGui::DragFloat("Far Plane", &farPlane, 1.0f, nearPlane + 0.1f, 10000.0f);

        ImGui::Separator();

        // Background color
        float bgColor[3] = { backgroundColor.r, backgroundColor.g, backgroundColor.b };
        if (ImGui::ColorEdit3("Background", bgColor))
        {
            backgroundColor = glm::vec3(bgColor[0], bgColor[1], bgColor[2]);
        }

        ImGui::Separator();

        // Display info
        ImGui::Text("Aspect Ratio: %.2f", aspectRatio);

        if (owner)
        {
            ComponentTransform* transform = owner->GetComponent<ComponentTransform>();
            if (transform)
            {
                glm::vec3 pos = transform->GetGlobalPosition();
                ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
            }
        }
    }
}
nlohmann::json ComponentCamera::GetProperties() const
{
    nlohmann::json props;

    props["fov"] = fov;
    props["nearPlane"] = nearPlane;
    props["farPlane"] = farPlane;
    props["aspectRatio"] = aspectRatio;
    props["projectionMode"] = (int)projectionMode;
    props["orthoSize"] = orthoSize;
    props["isMainCamera"] = isMainCamera;

    props["backgroundColor"] = {
        {"r", backgroundColor.r},
        {"g", backgroundColor.g},
        {"b", backgroundColor.b}
    };

    return props;
}

void ComponentCamera::SetProperties(const nlohmann::json& props)
{
    if (props.contains("fov"))
        fov = props["fov"];

    if (props.contains("nearPlane"))
        nearPlane = props["nearPlane"];

    if (props.contains("farPlane"))
        farPlane = props["farPlane"];

    if (props.contains("aspectRatio"))
        aspectRatio = props["aspectRatio"];

    if (props.contains("projectionMode"))
        projectionMode = (ProjectionMode)(int)props["projectionMode"];

    if (props.contains("orthoSize"))
        orthoSize = props["orthoSize"];

    if (props.contains("isMainCamera"))
        isMainCamera = props["isMainCamera"];

    if (props.contains("backgroundColor"))
    {
        backgroundColor.r = props["backgroundColor"]["r"];
        backgroundColor.g = props["backgroundColor"]["g"];
        backgroundColor.b = props["backgroundColor"]["b"];
    }
}