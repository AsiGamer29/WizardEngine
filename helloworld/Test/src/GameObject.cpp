#include "GameObject.h"
#include "BaseComponent.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GameObject::GameObject(const char* name)
    : name(name), active(true), parent(nullptr)
{
}

GameObject::~GameObject()
{
    // Limpiar componentes
    for (Component* comp : components)
    {
        delete comp;
    }
    components.clear();

    // No eliminamos hijos aquí, lo hace ModuleScene::RecursiveDelete
}

void GameObject::Update()
{
    if (!active)
        return;

    // Actualizar componentes
    for (Component* comp : components)
    {
        if (comp->IsActive())
        {
            comp->Update();
        }
    }

    // Actualizar hijos
    for (GameObject* child : children)
    {
        child->Update();
    }
}

Component* GameObject::CreateComponent(ComponentType type)
{
    Component* newComponent = nullptr;

    switch (type)
    {
    case ComponentType::TRANSFORM:
        newComponent = new ComponentTransform(this);
        break;

    case ComponentType::MESH:
        newComponent = new ComponentMesh(this);
        break;

    case ComponentType::MATERIAL:
        newComponent = new ComponentMaterial(this);
        break;

    default:
        std::cerr << "[GameObject] Unknown component type" << std::endl;
        return nullptr;
    }

    components.push_back(newComponent);
    return newComponent;
}

Component* GameObject::GetComponent(ComponentType type)
{
    for (Component* comp : components)
    {
        if (comp->GetType() == type)
            return comp;
    }
    return nullptr;
}

void GameObject::AddChild(GameObject* child)
{
    if (!child)
        return;

    // Si ya tenía padre, quitarlo de su lista de hijos
    if (child->parent)
    {
        child->parent->RemoveChild(child);
    }

    child->parent = this;
    children.push_back(child);
}

void GameObject::RemoveChild(GameObject* child)
{
    if (!child)
        return;

    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void GameObject::SetParent(GameObject* newParent)
{
    if (parent)
    {
        parent->RemoveChild(this);
    }

    if (newParent)
    {
        newParent->AddChild(this);
    }
}
void GameObject::AddComponent(Component* component)
{
    if (component == nullptr)
    {
        std::cerr << "[GameObject] Cannot add null component" << std::endl;
        return;
    }

    // Verificar que no exista ya
    auto it = std::find(components.begin(), components.end(), component);
    if (it != components.end())
    {
        std::cerr << "[GameObject] Component already exists in GameObject" << std::endl;
        return;
    }

    components.push_back(component);
}

void GameObject::UpdateAABB()
{
    globalAABB.Reset();

    if (!active)
        return;

    ComponentMesh* mesh = GetComponent<ComponentMesh>();
    ComponentTransform* transform = GetComponent<ComponentTransform>();

    if (mesh && transform)
    {
        // Obtener AABB local del mesh
        AABB localAABB = mesh->CalculateLocalAABB();

        if (localAABB.IsValid())
        {
            // Transformar AABB local a espacio global
            glm::mat4 globalMatrix = transform->GetGlobalMatrix();
            globalAABB = localAABB.Transform(globalMatrix);
        }
    }

    // Actualizar AABBs de los hijos y encapsularlos
    for (GameObject* child : children)
    {
        if (child && child->IsActive())
        {
            child->UpdateAABB();
            if (child->GetAABB().IsValid())
            {
                globalAABB.Encapsulate(child->GetAABB());
            }
        }
    }
}

bool GameObject::IntersectRayTriangles(const Ray& rayLocal, ComponentMesh* mesh,
    float& closestDist, glm::vec3& hitPoint)
{
    if (!mesh)
        return false;

    // CORREGIDO: Ahora usa MeshVertex en lugar de float[]
    const std::vector<MeshVertex>& vertices = mesh->GetVertices();
    const std::vector<unsigned int>& indices = mesh->GetIndices();

    if (vertices.empty() || indices.empty())
        return false;

    bool anyHit = false;
    closestDist = FLT_MAX;

    // Iterar sobre todos los triángulos
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Obtener índices de los 3 vértices del triángulo
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        // Asegurarse de que los índices son válidos
        if (idx0 >= vertices.size() ||
            idx1 >= vertices.size() ||
            idx2 >= vertices.size())
            continue;

        // CORREGIDO: Extraer posiciones directamente de MeshVertex
        glm::vec3 v0 = vertices[idx0].Position;
        glm::vec3 v1 = vertices[idx1].Position;
        glm::vec3 v2 = vertices[idx2].Position;

        // Algoritmo de intersección Möller-Trumbore
        const float EPSILON = 1e-8f;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(rayLocal.direction, edge2);
        float a = glm::dot(edge1, h);

        // Si a está cerca de 0, el rayo es paralelo al triángulo
        if (a > -EPSILON && a < EPSILON)
            continue;

        float f = 1.0f / a;
        glm::vec3 s = rayLocal.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f)
            continue;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(rayLocal.direction, q);

        if (v < 0.0f || u + v > 1.0f)
            continue;

        // Calcular t para encontrar el punto de intersección
        float t = f * glm::dot(edge2, q);

        // Solo aceptar intersecciones hacia adelante (t > 0) y más cercanas
        if (t > EPSILON && t < closestDist)
        {
            closestDist = t;
            hitPoint = rayLocal.GetPoint(t);
            anyHit = true;
        }
    }

    return anyHit;
}

bool GameObject::IntersectRay(const Ray& ray, RayHit& hit)
{
    if (!active)
        return false;

    // Test rápido contra AABB global
    float tMin, tMax;
    if (!globalAABB.IntersectRay(ray, tMin, tMax))
        return false;

    ComponentMesh* mesh = GetComponent<ComponentMesh>();
    ComponentTransform* transform = GetComponent<ComponentTransform>();

    if (!mesh || !transform)
        return false;

    // Transformar rayo de espacio global a espacio local del objeto
    glm::mat4 invTransform = glm::inverse(transform->GetGlobalMatrix());
    glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::vec3(invTransform * glm::vec4(ray.direction, 0.0f));
    Ray rayLocal(localOrigin, localDir);

    // Test de intersección contra triángulos
    float closestDist;
    glm::vec3 localHitPoint;

    if (IntersectRayTriangles(rayLocal, mesh, closestDist, localHitPoint))
    {
        // Convertir punto de impacto de vuelta a espacio global
        glm::mat4 globalMatrix = transform->GetGlobalMatrix();
        glm::vec3 worldHitPoint = glm::vec3(globalMatrix * glm::vec4(localHitPoint, 1.0f));

        // Calcular distancia real en espacio global
        float worldDist = glm::distance(ray.origin, worldHitPoint);

        // Actualizar hit si es más cercano que el anterior
        if (worldDist < hit.distance)
        {
            hit.hit = true;
            hit.distance = worldDist;
            hit.point = worldHitPoint;
            hit.gameObject = this;
            return true;
        }
    }

    return false;
}