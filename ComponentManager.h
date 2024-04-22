#ifndef MAIN_CPP_COMPONENTMANAGER_H
#define MAIN_CPP_COMPONENTMANAGER_H

#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "Actor.h"
#include "Input.h"
#include "Renderer.h"
#include "AudioManager.h"
#include "glm/glm.hpp"
#include "LuaMananger.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "box2d.h"
#include "Rigidbody.h"
#include "Raycast.h"
#include "EventBus.h"


class ComponentManager
{
public:
    static inline std::unordered_map<std::string, luabridge::LuaRef *> component_tables;

    // construct an Actor table for finding
    static inline std::vector<Actor *> actorTable = {};

    static inline std::vector<Actor *> actors_to_add;
    static inline std::vector<Actor *> actors_to_remove;
    static inline std::vector<Actor *> actors_not_to_destroy;

    ComponentManager()
    {
        Initialize(); // Ensure Lua state is initialized upon construction
    }

    ~ComponentManager()
    = default;

    static void Initialize();

    // Logging methods for lua
    static void Print(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    // Error logging method for lua
    static void PrintError(const std::string &message)
    {
        std::cerr << message << std::endl;
    }

    static void
    ApplyTemplateComponentOverrides(lua_State *L, const std::string &componentName,
                                    const rapidjson::Value &componentValue)
    {

        luabridge::LuaRef currentComponent = *ComponentManager::component_tables[componentName];

        for (auto it = componentValue.MemberBegin(); it != componentValue.MemberEnd(); ++it)
        {
            const std::string key = it->name.GetString();
            if (key == "type") continue; // Skip "type" since it's not a property to override

            const auto &val = it->value;

            // Check the type of the value and push it onto the Lua stack
            if (val.IsBool())
            {
                luabridge::push(L, val.GetBool()); // Push boolean
            }
            else if (val.IsNumber())
            {
                if (val.IsInt())
                {
                    luabridge::push(L, val.GetInt()); // Push integer
                }
                else if (val.IsDouble())
                {
                    luabridge::push(L, val.GetDouble()); // Push double
                }
                else
                {
                    continue; // Skip unknown or unsupported types
                }
            }
            else if (val.IsString())
            {
                luabridge::push(L, val.GetString()); // Push string
            }
            else
            {
                continue; // Skip unknown or unsupported types
            }

            // The top of the stack now has the value we just pushed
            auto *ref = new luabridge::LuaRef(luabridge::LuaRef::fromStack(L, -1));
            currentComponent[key] = *ref;

            lua_pop(L, 1); // Clean up the stack by removing the pushed value
        }
    }

    // Application related methods
    class Application
    {
    public:
        static void Quit()
        {
            exit(0);
        }

        static void Sleep(int milliseconds)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        }

        static int GetFrame()
        {
            // Assuming Helper::GetFrameNumber() is defined elsewhere
            return Helper::GetFrameNumber();
        }

        static void OpenURL(const std::string &url)
        {
#ifdef _WIN32
            std::string command = "start " + url;
#elif __APPLE__
            std::string command = "open " + url;
#else
            std::string command = "xdg-open " + url;
#endif
            std::system(command.c_str());
        }
    };

    static bool GetKeyWrapper(const std::string &keyName)
    {
        auto it = Input::__keycode_to_scancode.find(keyName);
        if (it != Input::__keycode_to_scancode.end())
        {
            return Input::GetKey(it->second);
        }
        return false;
    }

    static bool GetKeyDownWrapper(const std::string &keyName)
    {
        auto it = Input::__keycode_to_scancode.find(keyName);
        if (it != Input::__keycode_to_scancode.end())
        {
            return Input::GetKeyDown(it->second);
        }
        return false;
    }

    static bool GetKeyUpWrapper(const std::string &keyName)
    {
        auto it = Input::__keycode_to_scancode.find(keyName);
        if (it != Input::__keycode_to_scancode.end())
        {
            return Input::GetKeyUp(it->second);
        }
        return false;
    }

    static luabridge::LuaRef GetMousePositionWrapper(lua_State *L)
    {
        glm::vec2 pos = Input::GetMousePosition();
        luabridge::LuaRef table = luabridge::newTable(L);
        table["x"] = pos.x;
        table["y"] = pos.y;
        return table;
    }

    static void EstablishInheritance(luabridge::LuaRef &instance_table, luabridge::LuaRef &parent_table);

    static void InstantiateComponent(const std::string &name, const std::string &type, Actor *owner);

    // Create a new actor based on the specific actor template and return a reference to it
    static Actor *InstantiateActor(std::string actor_template_name)
    {
        // std::cout << "Frame " << Helper::GetFrameNumber() << ": Instantiating actor from template " << actor_template_name << std::endl;

        if (actor_template_name == "player"){
            std::cout << actor_template_name << std::endl;
        }
        
        std::string path = "resources/actor_templates/" + actor_template_name + ".template";
        // std::cout << "path: " << path << std::endl;
        auto *actor = new Actor(actor_template_name);

        rapidjson::Document doc;
        EngineUtils::ReadJsonFile(path, doc);

        if (doc.HasMember("name") && doc["name"].IsString())
        {
            actor->name = doc["name"].GetString();
        }

        std::map<std::string, luabridge::LuaRef *> components;

        if (doc.HasMember("components") && doc["components"].IsObject())
        {
            const auto &componentsObject = doc["components"].GetObject();
            for (const auto &component: componentsObject)
            {
                const std::string componentName = component.name.GetString();
                const std::string componentType = component.value["type"].GetString();

                std::string componentPath = "resources/component_types/" + componentType + ".lua";

                if (componentType != "Rigidbody")
                {
                    // Check if the component Lua file exists
                    if (!std::filesystem::exists(componentPath))
                    {
                        std::cout << "error: failed to locate component " << componentType;
                        exit(0);
                    }

                    const std::string path = "resources/component_types/" + componentType + ".lua";

                    if (luaL_dofile(LuaManager::lua_state, path.c_str()) != LUA_OK)
                    {
                        std::cout << "problem with lua file " << componentType;
                        exit(0);
                    }

                    // Get the base table for the component from the global Lua environment
                    luabridge::LuaRef base_table = luabridge::getGlobal(LuaManager::lua_state, componentType.c_str());

                    // Create a new empty table to represent the new component instance
                    luabridge::LuaRef instance_table = luabridge::newTable(LuaManager::lua_state);

                    // Add 'self.key' to the new instance table
                    instance_table["key"] = componentName;

                    // Add 'self.type' to the new instance table
                    instance_table["type"] = componentType;

                    // Add 'enabled' to enable the OnStart, OnUpdate, OnLateUpdate
                    instance_table["enabled"] = true;

                    // Establish inheritance from the base table to the new table
                    EstablishInheritance(instance_table, base_table);

                    // Inject the actor reference
                    instance_table["actor"] = actor;

                    // for checking removed components
                    instance_table["removed"] = false;

                    // Store the new table in the component_tables map
                    component_tables[componentName] = new luabridge::LuaRef(instance_table);

                    components.insert(
                            {componentName, ComponentManager::component_tables[componentName]});

                    // Apply overrides to set the new sprite name or the variable to Transform
                    ApplyTemplateComponentOverrides(LuaManager::lua_state, componentName, component.value);
                }
                else
                {

                    auto *rigidbody = new Rigidbody();

                    luabridge::push(LuaManager::lua_state, rigidbody);

                    auto *componentRef = new luabridge::LuaRef(luabridge::LuaRef::fromStack(LuaManager::lua_state, -1));

                    (*componentRef)["actor"] = actor;
                    (*componentRef)["type"] = componentType;
                    (*componentRef)["key"] = componentName;
                    (*componentRef)["enabled"] = true;
                    (*componentRef)["removed"] = false;

                    component_tables[componentName] = componentRef;
                    components.insert({componentName, componentRef});

                    ApplyTemplateComponentOverrides(LuaManager::lua_state, componentName, component.value);

                    if ((*componentRef)["Ready"].isFunction())
                    {
                        actor->componentsOnReady.push_back(
                                componentRef);
                        // Main reason should be the way that we run the OnStart for those actors_to_add

                    }
                }

            }

            actor->components = std::move(components);

            // Add the newly created component to the actor's component list for componentsOnStart and componentsOnUpdate
            for (const auto &component: actor->components)
            {
                luabridge::LuaRef onUpdate = (*component.second)["OnUpdate"];
                luabridge::LuaRef onStart = (*component.second)["OnStart"];
                if (onUpdate.isFunction() && (*component.second)["enabled"] && !(*component.second)["removed"])
                {
                    actor->componentsOnUpdate.insert(component);
                }
                if (onStart.isFunction() && (*component.second)["enabled"] && !(*component.second)["removed"])
                {
                    actor->componentsOnStart.insert(component);
                }
            }

            actorTable.push_back(actor);
            actors_to_add.push_back(actor);
        }

        return actor;
    };

    static void DestroyActor(Actor *actor)
    {
        actors_to_remove.push_back(actor);

        // remove it from the actors_to_add - for the case that the actor is added and removed in the same frame
        actors_to_add.erase(std::remove(actors_to_add.begin(), actors_to_add.end(), actor), actors_to_add.end());

        for (const auto &component: actor->components)
        {
            (*component.second)["removed"] = true;
        }
        actorTable.erase(std::remove(actorTable.begin(), actorTable.end(), actor), actorTable.end());
    }

    static void DontDestroyActor(Actor *actor)
    {
        // actor->fromAnotherScene = true; // set the actor to be from another scene
        actor->dontDestroyOnLoad = true; // set the actor to not be destroyed when loading a new scene
        // Don't call the Start function on actors that changed scenes, only on those loaded in the new scenes
        actors_not_to_destroy.push_back(actor);
    }

    static Actor *FindActorByName(const std::string &name)
    {
        for (auto &actor: actorTable)
        {
            if (actor->GetName() == name)
            {
                return actor;
            }
        }

        return luabridge::LuaRef(LuaManager::lua_state); // returns nil to lua
    };

    // Actor.FindAll(name) should return all actors with the provided name
    // return in the form of an indexed table that may be iterated through with ipairs()
    // return an empty table if no actors with the desired name exist
    static luabridge::LuaRef *FindAllActorsByName(const std::string &name)
    {
        luabridge::LuaRef *table = new luabridge::LuaRef(
                luabridge::newTable(component_tables.begin()->second->state()));

        int index = 1;

        for (auto &actor: actorTable)
        {
            if (actor->GetName() == name)
            {
                (*table)[index] = actor;
                index++;
            }
        }
        return table;
    };

    static luabridge::LuaRef *CreateRigidbody(const std::string &name, Actor *actor);

};

void ComponentManager::Initialize()
{
    LuaManager::lua_state = luaL_newstate(); // create a Lua state
    luaL_openlibs(LuaManager::lua_state); // load libs

    // Registering Debug namespace
    // with logging functions
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Debug")
            .addFunction("Log", ComponentManager::Print)
            .addFunction("LogError", ComponentManager::PrintError)
            .endNamespace();

    // glm::vec2 instances
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<glm::vec2>("vec2")
            .addData("x", &glm::vec2::x)
            .addData("y", &glm::vec2::y)
            .endClass();
    // new_component_table["pos"] = glm::vec2(311, 305);

    // Registering Actor class
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<Actor>("Actor")
            .addFunction("GetName", &Actor::GetName)
            .addFunction("GetID", &Actor::GetID)
            .addFunction("GetComponentByKey", &Actor::GetComponentByKey)
            .addFunction("GetComponent", &Actor::GetComponentByType)
            .addFunction("GetComponents", &Actor::GetComponents)
            .addFunction("AddComponent", &Actor::AddComponent)
            .addFunction("RemoveComponent", &Actor::RemoveComponent)
            .endClass();

    // Registering Application class
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Application")
            .addFunction("Quit", &Application::Quit)
            .addFunction("Sleep", &Application::Sleep)
            .addFunction("GetFrame", &Application::GetFrame)
            .addFunction("OpenURL", &Application::OpenURL)
            .endNamespace();

    // Registering Actor namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Actor")
            .addFunction("Find", ComponentManager::FindActorByName)
            .addFunction("FindAll", ComponentManager::FindAllActorsByName)
            .addFunction("Instantiate", ComponentManager::InstantiateActor)
            .addFunction("Destroy", ComponentManager::DestroyActor)
            .endNamespace();

    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Input")
            .addFunction("GetKey", &GetKeyWrapper)
            .addFunction("GetKeyDown", &GetKeyDownWrapper)
            .addFunction("GetKeyUp", &GetKeyUpWrapper)
            .addFunction("GetMousePosition", &GetMousePositionWrapper)
            .addFunction("GetMouseButton", &Input::GetMouseButton)
            .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
            .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
            .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
            .endNamespace();

    // Registering RendererText namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Text")
            .addFunction("Draw", &Renderer::ReadTextRenderRequest)
            .endNamespace();

    // Registering RendererImage namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Image")
            .addFunction("DrawUI", &Renderer::ReadUIRenderRequest)
            .addFunction("DrawUIEx", &Renderer::ReadUIRenderRequestEx)
            .addFunction("Draw", &Renderer::ReadImageRenderRequest)
            .addFunction("DrawEx", &Renderer::ReadImageRenderRequestEx)
            .addFunction("DrawPixel", &Renderer::ReadPixelRenderRequest)
            .endNamespace();

    // Registering Camera namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Camera")
            .addFunction("SetPosition", &Renderer::Camera::SetCameraPosition)
            .addFunction("GetPositionX", &Renderer::Camera::GetPositionX)
            .addFunction("GetPositionY", &Renderer::Camera::GetPositionY)
            .addFunction("SetZoom", &Renderer::Camera::SetZoomFactor)
            .addFunction("GetZoom", &Renderer::Camera::GetZoomFactor)
            .endNamespace();

    // Registering Audio namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Audio")
            .addFunction("Play", &AudioManager::PlayMusic)
            .addFunction("Halt", &AudioManager::StopMusic)
            .addFunction("SetVolume", &AudioManager::SetVolume)
            .endNamespace();

    // Registering Scene namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Scene")
            .addFunction("Load", &LuaManager::LoadScene)
            .addFunction("GetCurrent", &LuaManager::GetCurrentSceneName)
            .addFunction("DontDestroy", &ComponentManager::DontDestroyActor)
            .endNamespace();

    // Registering Box2D namespace
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<b2Vec2>("Vector2")
            .addConstructor < void(*)
    (float, float) > ()
            .addData("x", &b2Vec2::x)
            .addData("y", &b2Vec2::y)
            .addFunction("Normalize", &b2Vec2::Normalize)
            .addFunction("Length", &b2Vec2::Length)
            .addFunction("__add", &b2Vec2::operator_add)
            .addFunction("__sub", &b2Vec2::operator_sub)
            .addFunction("__mul", &b2Vec2::operator_mul)
            .endClass()

                    // Registering static functions for Vector2 table
            .beginNamespace("Vector2")
            .addFunction("Distance", &b2Distance)
                    // Correctly casting the overloaded b2Dot function to add it as a static function
            .addFunction("Dot", static_cast<float (*)(const b2Vec2 &, const b2Vec2 &)>(&b2Dot))
            .endNamespace();

    // Registering Rigidbody class
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<Rigidbody>("Rigidbody")
            .addData("x", &Rigidbody::x)
            .addData("y", &Rigidbody::y)
            .addData("width", &Rigidbody::width)
            .addData("height", &Rigidbody::height)
            .addData("radius", &Rigidbody::radius)
            .addData("friction", &Rigidbody::friction)
            .addData("bounciness", &Rigidbody::bounciness)
            .addData("enabled", &Rigidbody::enabled)
            .addData("removed", &Rigidbody::removed)
            .addData("key", &Rigidbody::key)
            .addData("type", &Rigidbody::componentType)
            .addData("body_type", &Rigidbody::bodyType)
            .addData("gravity_scale", &Rigidbody::gravity_scale)
            .addData("density", &Rigidbody::density)
            .addData("angular_friction", &Rigidbody::angular_friction)
            .addData("rotation", &Rigidbody::rotation)
            .addData("has_collider", &Rigidbody::has_collider)
            .addData("collider_type", &Rigidbody::collider_type)
            .addData("has_trigger", &Rigidbody::has_trigger)
            .addData("trigger_type", &Rigidbody::trigger_type)
            .addData("trigger_width", &Rigidbody::trigger_width)
            .addData("trigger_height", &Rigidbody::trigger_height)
            .addData("trigger_radius", &Rigidbody::trigger_radius)
            .addData("actor", &Rigidbody::actor)
            .addFunction("GetPosition", &Rigidbody::GetBodyPosition)
            .addFunction("GetRotation", &Rigidbody::GetBodyRotation)
            .addFunction("Ready", &Rigidbody::Ready)
            .addFunction("AddForce", &Rigidbody::AddForce)
            .addFunction("SetVelocity", &Rigidbody::SetVelocity)
            .addFunction("SetPosition", &Rigidbody::SetPosition)
            .addFunction("SetRotation", &Rigidbody::SetRotation)
            .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
            .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
            .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
            .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
            .addFunction("GetVelocity", &Rigidbody::GetVelocity)
            .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
            .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
            .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
            .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
            .endClass();

    // Registering Collision Class as C++ class in LuaBridge
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<Actor::Collision>("Collision")
            .addData("other", &Actor::Collision::other)
            .addData("point", &Actor::Collision::point)
            .addData("relative_velocity", &Actor::Collision::relative_velocity)
            .addData("normal", &Actor::Collision::normal)
            .endClass();

    // Registering HitResult class
    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginClass<HitResult>("HitResult")
                    // .addConstructor<void(*)(Actor*, b2Vec2, b2Vec2, bool, float)>()
            .addData("actor",
                     &HitResult::actor) // Assuming you have a getter that converts Actor* to a Lua-friendly form
            .addData("point", &HitResult::point) // Assuming a getter that returns a table or Vector2 for Lua
            .addData("normal", &HitResult::normal) // Similar assumption as above
            .addData("is_trigger", &HitResult::is_trigger)
            .addData("fraction", &HitResult::fraction)
            .endClass();


    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Physics")
            .addFunction("Raycast", &Physics::Raycast)
            .addFunction("RaycastAll", &Physics::RaycastAll)
            .endNamespace();

    luabridge::getGlobalNamespace(LuaManager::lua_state)
            .beginNamespace("Event")
            .addFunction("Subscribe", &EventBus::Subscribe)
            .addFunction("Unsubscribe", &EventBus::UnSubscribe)
            .addFunction("Publish", &EventBus::Publish)
            .endNamespace();

}

// “instance” component types like these by creating a new empty Lua table
// and establishing inheritance from the base table to the new table.
void ComponentManager::EstablishInheritance(luabridge::LuaRef &instance_table, luabridge::LuaRef &parent_table)
{
    // We must create a metatable to establish inheritance in Lua
    luabridge::LuaRef new_metatable = luabridge::newTable(LuaManager::lua_state);
    new_metatable["__index"] = parent_table;

    // use the raw lua C-API (lua stack) to perform a "setmetatable" operation
    instance_table.push(LuaManager::lua_state);
    new_metatable.push(LuaManager::lua_state);
    lua_setmetatable(LuaManager::lua_state, -2);
    lua_pop(LuaManager::lua_state, 1);
}

void ComponentManager::InstantiateComponent(const std::string &name, const std::string &type, Actor *owner)
{
    const std::string path = "resources/component_types/" + type + ".lua";

    if (luaL_dofile(LuaManager::lua_state, path.c_str()) != LUA_OK)
    {
        std::cout << "problem with lua file " << type;
        exit(0);
    }

    // Get the base table for the component from the global Lua environment
    luabridge::LuaRef base_table = luabridge::getGlobal(LuaManager::lua_state, type.c_str());

    // Create a new empty table to represent the new component instance
    luabridge::LuaRef instance_table = luabridge::newTable(LuaManager::lua_state);

    // Add 'self.key' to the new instance table
    instance_table["key"] = name;

    // Add 'self.type' to the new instance table
    instance_table["type"] = type;

//    All components must have a special enabled variable that begins true.
//    If false, no lifecycle function will run (OnStart, OnUpdate, etc).
//    Note : The OnStart function does not run again if a component is re-enabled.
    instance_table["enabled"] = true;

    // Establish inheritance from the base table to the new table
    EstablishInheritance(instance_table, base_table);

    // Inject the actor reference
    instance_table["actor"] = owner;

    // for checking removed components
    instance_table["removed"] = false;

    // Store the new table in the component_tables map
    component_tables[name] = new luabridge::LuaRef(instance_table);
}

luabridge::LuaRef *ComponentManager::CreateRigidbody(const std::string &name, Actor *actor)
{
    // Create a new Rigidbody instance.
    auto *rigidbody = new Rigidbody(); // Consider managing this memory more safely, e.g., with smart pointers.

    // Push the new Rigidbody instance onto the Lua stack as a full userdata.
    // LuaBridge knows how to handle this because we've registered the Rigidbody class.
    luabridge::push(LuaManager::lua_state, rigidbody);

    auto *componentRef = new luabridge::LuaRef(luabridge::LuaRef::fromStack(LuaManager::lua_state, -1));

    (*componentRef)["actor"] = actor;

    component_tables[name] = componentRef;

    return componentRef; // Return the table wrapped in a LuaRef.
}


#endif //MAIN_CPP_COMPONENTMANAGER_H
