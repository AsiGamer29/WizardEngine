#include "GameObject.h"
#include "BaseComponent.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "AABB.h"
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

    ComponentTransform* transform = GetComponent<ComponentTransform>();
    ComponentMesh* mesh = GetComponent<ComponentMesh>();

    if (mesh && transform)
    {
        // Obtener AABB local del mesh (ya calculado)
        AABB localAABB = mesh->GetLocalAABB();

        if (localAABB.IsValid())
        {
            // Transformar el AABB local a espacio world
            glm::mat4 worldMatrix = transform->GetGlobalMatrix();
            AABB worldAABB = localAABB.Transform(worldMatrix);

            // Expandir nuestro AABB con el AABB transformado
            globalAABB.Encapsulate(worldAABB);
        }
    }

    // Actualizar recursivamente todos los hijos
    for (GameObject* child : children)
    {
        if (child)
        {
            child->UpdateAABB();

            const AABB& childAABB = child->GetAABB();
            if (childAABB.IsValid())
            {
                globalAABB.Encapsulate(childAABB);
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
    const std::vector<MeshVertex>& vertices = mesh->GetMeshVertices();
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

bool GameObject::IntersectRay(const Ray& ray, RayHit& outHit)
{
    // 1. TEST CONTRA AABB
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t0 = (globalAABB.min - ray.origin) * invDir;
    glm::vec3 t1 = (globalAABB.max - ray.origin) * invDir;

    glm::vec3 tmin = glm::min(t0, t1);
    glm::vec3 tmax = glm::max(t0, t1);

    float tNear = glm::max(glm::max(tmin.x, tmin.y), tmin.z);
    float tFar = glm::min(glm::min(tmax.x, tmax.y), tmax.z);

    // Si no intersecta el AABB, salir
    if (tNear > tFar || tFar < 0.0f)
        return false;

    // 2. SI INTERSECTA AABB, TEST CONTRA TRIÁNGULOS
    ComponentMesh* mesh = GetComponent<ComponentMesh>();
    if (!mesh)
        return false;

    ComponentTransform* transform = GetComponent<ComponentTransform>();
    if (!transform)
        return false;

    // 3. TRANSFORMAR RAY A ESPACIO LOCAL
    glm::mat4 worldMatrix = transform->GetGlobalMatrix();
    glm::mat4 invWorld = glm::inverse(worldMatrix);

    glm::vec3 localOrigin = glm::vec3(invWorld * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDirection = glm::vec3(invWorld * glm::vec4(ray.direction, 0.0f));
    Ray localRay(localOrigin, localDirection);

    // 4. OBTENER DATOS DEL MESH
    const std::vector<float>& vertices = mesh->GetVertices();
    const std::vector<unsigned int>& indices = mesh->GetIndices();

    if (vertices.empty() || indices.empty())
        return false;

    // 5. TEST CONTRA CADA TRIÁNGULO
    // Formato: [x, y, z, nx, ny, nz, u, v] = 8 floats por vértice
    const size_t VERTEX_STRIDE = 8;

    float closestDistance = FLT_MAX;
    bool foundHit = false;

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int idx0 = indices[i];
        unsigned int idx1 = indices[i + 1];
        unsigned int idx2 = indices[i + 2];

        // Obtener posiciones de los vértices del triángulo
        size_t offset0 = idx0 * VERTEX_STRIDE;
        size_t offset1 = idx1 * VERTEX_STRIDE;
        size_t offset2 = idx2 * VERTEX_STRIDE;

        glm::vec3 v0(vertices[offset0], vertices[offset0 + 1], vertices[offset0 + 2]);
        glm::vec3 v1(vertices[offset1], vertices[offset1 + 1], vertices[offset1 + 2]);
        glm::vec3 v2(vertices[offset2], vertices[offset2 + 1], vertices[offset2 + 2]);

        // ALGORITMO MÖLLER-TRUMBORE para intersección ray-triángulo
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(localRay.direction, edge2);
        float a = glm::dot(edge1, h);

        // Rayo paralelo al triángulo
        if (a > -0.00001f && a < 0.00001f)
            continue;

        float f = 1.0f / a;
        glm::vec3 s = localRay.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f)
            continue;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(localRay.direction, q);

        if (v < 0.0f || u + v > 1.0f)
            continue;

        // Calcular distancia
        float t = f * glm::dot(edge2, q);

        if (t > 0.00001f && t < closestDistance)
        {
            closestDistance = t;
            foundHit = true;
        }
    }

    // 6. LLENAR RESULTADO
    if (foundHit)
    {
        outHit.hit = true;
        outHit.distance = closestDistance;
        outHit.point = localRay.GetPoint(closestDistance);
        outHit.gameObject = this;
        return true;
    }

    return false;
}