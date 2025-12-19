#include "GameObject.h"
#include "Application.h"
#include "ComponentTransform.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "AABB.h"
#include "Texture.h"
#include "Ray.h"
#include <algorithm>
#include <nlohmann/json.hpp>

GameObject::GameObject(const char* name, GameObject* parent)
    : name(name)
    , active(true)
    , parent(parent)
    , hasAABB(false)
    , uuid(UUID::Generate())
{
    CreateComponent(ComponentType::TRANSFORM);

    if (parent)
    {
        parent->AddChild(this);
    }
}

GameObject::~GameObject()
{
    for (Component* comp : components)
    {
        delete comp;
    }
    components.clear();

    children.clear();
}

void GameObject::Update()
{
    if (!active) return;

    // Actualizar todos los componentes
    for (Component* comp : components)
    {
        if (comp && comp->IsActive())
        {
            comp->Update();
        }
    }

    // Actualizar hijos recursivamente
    for (GameObject* child : children)
    {
        if (child)
        {
            child->Update();
        }
    }
}

Component* GameObject::CreateComponent(ComponentType type)
{
    Component* newComp = nullptr;

    switch (type)
    {
    case ComponentType::TRANSFORM:
        newComp = new ComponentTransform(this);
        break;
    case ComponentType::MESH:
        newComp = new ComponentMesh(this);
        break;
    case ComponentType::MATERIAL:
        newComp = new ComponentMaterial(this);
        break;
    default:
        return nullptr;
    }

    if (newComp)
    {
        components.push_back(newComp);
    }

    return newComp;
}

void GameObject::SetParent(GameObject* newParent)
{
    if (parent == newParent)
    {
        return;
    }

    if (parent)
    {
        parent->RemoveChild(this);
    }

    parent = newParent;

    if (parent)
    {
        parent->AddChild(this);
    }

    ComponentTransform* transform = GetComponent<ComponentTransform>();
    if (transform)
    {
        transform->MarkGlobalDirty();
    }
}

void GameObject::AddChild(GameObject* child)
{
    if (child && std::find(children.begin(), children.end(), child) == children.end())
    {
        children.push_back(child);
    }
}

void GameObject::RemoveChild(GameObject* child)
{
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end())
    {
        children.erase(it);
    }
}

void GameObject::UpdateAABB()
{
    ComponentMesh* mesh = GetComponent<ComponentMesh>();
    ComponentTransform* transform = GetComponent<ComponentTransform>();

    if (!mesh || !transform)
    {
        hasAABB = false;
        return;
    }

    AABB localAABB = mesh->GetLocalAABB();
    if (localAABB.min == glm::vec3(0.0f) && localAABB.max == glm::vec3(0.0f))
    {
        hasAABB = false;
        return;
    }

    glm::mat4 globalMatrix = transform->GetGlobalMatrix();

    glm::vec3 corners[8] = {
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.max.z)
    };

    glm::vec3 transformedCorners[8];
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 worldPos = globalMatrix * glm::vec4(corners[i], 1.0f);
        transformedCorners[i] = glm::vec3(worldPos);
    }

    glm::vec3 newMin = transformedCorners[0];
    glm::vec3 newMax = transformedCorners[0];

    for (int i = 1; i < 8; ++i)
    {
        newMin = glm::min(newMin, transformedCorners[i]);
        newMax = glm::max(newMax, transformedCorners[i]);
    }

    aabb.min = newMin;
    aabb.max = newMax;
    hasAABB = true;
}

void GameObject::SetAABB(const AABB& newAABB)
{
    aabb = newAABB;
    hasAABB = true;
}

bool GameObject::IntersectRay(const Ray& ray, RayHit& hit)
{
    if (!hasAABB || !active)
    {
        return false;
    }

    glm::vec3 tMin = (aabb.min - ray.origin) / ray.direction;
    glm::vec3 tMax = (aabb.max - ray.origin) / ray.direction;

    glm::vec3 t1 = glm::min(tMin, tMax);
    glm::vec3 t2 = glm::max(tMin, tMax);

    float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);

    if (tNear > tFar || tFar < 0.0f)
    {
        return false;
    }

    float t = (tNear > 0.0f) ? tNear : tFar;

    if (t < hit.distance)
    {
        hit.hit = true;
        hit.distance = t;
        hit.point = ray.GetPoint(t);
        hit.gameObject = this;
        return true;
    }

    return false;
}

// ============================================
// SERIALIZATION
// ============================================

nlohmann::json GameObject::Serialize() const
{
    nlohmann::json j;

    j["UID"] = uuid.GetValue();
    j["Name"] = name;
    j["Active"] = active;

    // Parent UID
    if (parent)
    {
        j["ParentUID"] = parent->GetUUID().GetValue();
    }
    else
    {
        j["ParentUID"] = 0;
    }

    // Transform component (siempre existe)
    ComponentTransform* transform = GetComponent<ComponentTransform>();
    if (transform)
    {
        glm::vec3 pos = transform->GetPosition();
        glm::vec3 scale = transform->GetScale();
        glm::quat rot = transform->GetRotation();

        j["Translation"] = { pos.x, pos.y, pos.z };
        j["Scale"] = { scale.x, scale.y, scale.z };
        j["Rotation"] = { rot.w, rot.x, rot.y, rot.z };
    }

    // Components array
    nlohmann::json componentsArray = nlohmann::json::array();

    for (Component* comp : components)
    {
        if (!comp) continue;

        nlohmann::json compJson;

        if (comp->GetType() == ComponentType::MESH)
        {
            ComponentMesh* mesh = static_cast<ComponentMesh*>(comp);
            compJson["Type"] = "Mesh";

            std::string sourcePath = mesh->GetSourceAssetPath();

            std::cout << "[GameObject::Serialize] Mesh for '" << name << "'" << std::endl;
            std::cout << "  SourceAssetPath: " << (sourcePath.empty() ? "(empty)" : sourcePath) << std::endl;

            // ESTRATEGIA DUAL: Guardar AMBOS - referencia Y datos
            // Esto asegura compatibilidad y recuperación ante errores

            if (!sourcePath.empty() && std::filesystem::exists(sourcePath))
            {
                // Guardar referencia al archivo .wzm
                compJson["SourceAssetPath"] = sourcePath;
                std::cout << "  -> Saved reference to: " << sourcePath << std::endl;
            }

            // SIEMPRE guardar también la geometría como backup
            // Esto permite cargar la escena incluso si los archivos .wzm se mueven
            if (mesh->GetVertexCount() > 0)
            {
                compJson["MeshData"] = mesh->SerializeMesh();
                std::cout << "  -> Saved mesh data (" << mesh->GetVertexCount() << " vertices)" << std::endl;
            }
            else
            {
                std::cout << "  -> WARNING: No vertex data to save!" << std::endl;
            }
        }
        else if (comp->GetType() == ComponentType::MATERIAL)
        {
            ComponentMaterial* mat = static_cast<ComponentMaterial*>(comp);
            compJson["Type"] = "Material";

            const char* path = mat->GetTexturePath();
            compJson["TexturePath"] = path ? path : "";
            compJson["Width"] = mat->GetWidth();
            compJson["Height"] = mat->GetHeight();

            // Guardar propiedades de alpha
            compJson["AlphaMode"] = (int)mat->GetAlphaMode();
            compJson["AlphaCutoff"] = mat->GetAlphaCutoff();
            compJson["BlendMode"] = (int)mat->GetBlendMode();
        }

        if (!compJson.empty())
        {
            componentsArray.push_back(compJson);
        }
    }

    j["Components"] = componentsArray;

    return j;
}

void GameObject::Deserialize(const nlohmann::json& json)
{
    if (json.contains("UID"))
    {
        uuid = UUID(json["UID"].get<uint32_t>());
    }

    if (json.contains("Name"))
    {
        name = json["Name"].get<std::string>();
    }

    if (json.contains("Active"))
    {
        active = json["Active"].get<bool>();
    }

    // Transform
    ComponentTransform* transform = GetComponent<ComponentTransform>();
    if (transform)
    {
        if (json.contains("Translation"))
        {
            auto trans = json["Translation"];
            transform->SetPosition(glm::vec3(trans[0], trans[1], trans[2]));
        }

        if (json.contains("Scale"))
        {
            auto scale = json["Scale"];
            transform->SetScale(glm::vec3(scale[0], scale[1], scale[2]));
        }

        if (json.contains("Rotation"))
        {
            auto rot = json["Rotation"];
            transform->SetRotation(glm::quat(rot[0], rot[1], rot[2], rot[3]));
        }
    }

    // Components
    if (json.contains("Components"))
    {
        const auto& componentsArray = json["Components"];

        for (const auto& compJson : componentsArray)
        {
            if (!compJson.contains("Type")) continue;

            std::string type = compJson["Type"].get<std::string>();

            if (type == "Mesh")
            {
                std::cout << "\n[GameObject::Deserialize] Loading Mesh for: " << name << std::endl;

                ComponentMesh* mesh = GetComponent<ComponentMesh>();
                if (!mesh)
                {
                    mesh = static_cast<ComponentMesh*>(CreateComponent(ComponentType::MESH));
                }

                if (!mesh)
                {
                    std::cerr << "  ERROR: Failed to create ComponentMesh!" << std::endl;
                    continue;
                }

                bool meshLoaded = false;

                // PRIORIDAD 1: Intentar cargar desde SourceAssetPath (archivo .wzm)
                if (compJson.contains("SourceAssetPath"))
                {
                    std::string meshPath = compJson["SourceAssetPath"].get<std::string>();

                    if (!meshPath.empty() && std::filesystem::exists(meshPath))
                    {
                        std::cout << "  Attempting to load from: " << meshPath << std::endl;

                        WizardEngine::WizardMeshData meshData;
                        if (WizardEngine::MeshImporter::Load(meshPath, meshData))
                        {
                            MeshGeometry geom;
                            geom.vertices.reserve(meshData.vertices.size());

                            for (const auto& v : meshData.vertices)
                            {
                                GeomVertex gv;
                                gv.Position = v.position;
                                gv.Normal = v.normal;
                                gv.TexCoords = v.texCoords;
                                geom.vertices.push_back(gv);
                            }

                            geom.indices = meshData.indices;
                            mesh->LoadFromGeometry(&geom);
                            mesh->SetSourceAssetPath(meshPath);

                            std::cout << "  Loaded from .wzm: " << meshData.vertices.size() << " vertices" << std::endl;
                            meshLoaded = true;
                        }
                        else
                        {
                            std::cerr << "  WARNING: Failed to load .wzm file" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "  WARNING: .wzm file not found or path empty" << std::endl;
                    }
                }

                // PRIORIDAD 2: FALLBACK - Cargar desde MeshData embebido
                if (!meshLoaded && compJson.contains("MeshData"))
                {
                    std::cout << "  Loading from embedded MeshData (fallback)..." << std::endl;

                    try
                    {
                        mesh->DeserializeMesh(compJson["MeshData"]);
                        std::cout << "  Loaded from embedded data: " << mesh->GetVertexCount() << " vertices" << std::endl;
                        meshLoaded = true;
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "  ERROR: Failed to deserialize mesh data: " << e.what() << std::endl;
                    }
                }

                if (!meshLoaded)
                {
                    std::cerr << "  ERROR: Could not load mesh from any source!" << std::endl;
                }
            }
            else if (type == "Material")
            {
                ComponentMaterial* mat = GetComponent<ComponentMaterial>();
                if (!mat)
                {
                    mat = static_cast<ComponentMaterial*>(CreateComponent(ComponentType::MATERIAL));
                }

                if (mat && compJson.contains("TexturePath"))
                {
                    std::string texPath = compJson["TexturePath"].get<std::string>();

                    if (!texPath.empty() && texPath != "checkerboard_default")
                    {
                        mat->LoadTexture(texPath.c_str());
                    }
                    else
                    {
                        GLuint checkerTex = Texture::CreateCheckerboardTexture(512, 512, 32);
                        mat->SetTexture(checkerTex, "checkerboard_default", 3);
                    }

                    // Restaurar propiedades de alpha
                    if (compJson.contains("AlphaMode"))
                    {
                        mat->SetAlphaMode((AlphaMode)compJson["AlphaMode"].get<int>());
                    }

                    if (compJson.contains("AlphaCutoff"))
                    {
                        mat->SetAlphaCutoff(compJson["AlphaCutoff"].get<float>());
                    }

                    if (compJson.contains("BlendMode"))
                    {
                        mat->SetBlendMode((BlendMode)compJson["BlendMode"].get<int>());
                    }
                }
            }
        }
    }
}

void GameObject::ClearHierarchyReferences()
{
    parent = nullptr;
    children.clear();
}