#ifndef MAIN_CPP_ACTOR_H
#define MAIN_CPP_ACTOR_H

#include <string>
#include <optional>
#include <map>
#include <vector>
#include <utility>
#include "glm/glm.hpp"
#include "rapidjson/document.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "LuaMananger.h"
#include "box2d.h"

static int next_actor_id = 1; // Global counter to ensure unique actor IDs

static int component_id = 0; // Global counter to record the number of times AddComponent()

static inline bool world_initialized = false;
static inline b2World *world = nullptr;

static inline std::vector<luabridge::LuaRef *> componentsAwaitingOnStart; // Global vector to store components awaiting OnStart()

class Actor
{
public:
    luabridge::LuaRef *AddComponent(const std::string &type);

    int actor_id;
    std::string name;
    bool fromAnotherScene; // flag to determine whether this actor come from another scene
    bool dontDestroyOnLoad; // flag to determine whether this actor should be destroyed when scene changes
    // Components should be processed in the alphabetical order of their key
    static inline std::map<std::string, luabridge::LuaRef *> componentsAdded; // just_added_components

    std::map<std::string, luabridge::LuaRef *> componentsOnStart; // components_on_start
    std::map<std::string, luabridge::LuaRef *> componentsOnUpdate; // components_on_update
    std::vector<luabridge::LuaRef *> componentsOnReady; // for those newly added rigidbody components
//    std::map<std::string, luabridge::LuaRef *> componentsOnLateUpdate; // components_on_late_update
    std::map<std::string, luabridge::LuaRef *> componentsOnTriggerEnter; // components_on_trigger_enter
    std::map<std::string, luabridge::LuaRef *> componentsOnTriggerExit; // components_on_trigger_exit
    std::map<std::string, luabridge::LuaRef *> componentsOnCollisionEnter; // components_on_collision_enter
    std::map<std::string, luabridge::LuaRef *> componentsOnCollisionExit; // components_on_collision_exit

    std::vector<std::string> componentsToRemove; // components_to_remove
    std::map<std::string, luabridge::LuaRef *> components; // Key is the component key, value is the component reference

    class Collision
    {
    public:
        Actor *other; // the other actor involved in the collision
        b2Vec2 point; // First point of collision - use GetWorldManifold() to get this
        b2Vec2 relative_velocity; // body_a velocity - body_b velocity
        b2Vec2 normal; // use GetWorldManifold().normal to get this
    };

    // Constructors
    explicit Actor(std::string name, bool fromAnotherScene = false, bool dontDestroyOnLoad = false) :
            actor_id(next_actor_id++), name(std::move(name)), fromAnotherScene(fromAnotherScene),
            dontDestroyOnLoad(dontDestroyOnLoad) {}

    Actor() : actor_id(next_actor_id++), fromAnotherScene(false), dontDestroyOnLoad(false) {} // Default constructor

    // ~Actor() { std::cout << "ACTOR DESTROYED OH NOOOOOOO" << std::endl; }

    // Method to update an actor from .scene file, excluding actor_id
    void updateFromJson(const rapidjson::Value &actorValue)
    {
        if (actorValue.HasMember("name")) name = actorValue["name"].GetString();
    }

    std::string GetName() const
    {
        return name;
    }

    int GetID() const
    {
        return actor_id;
    }

    // self.actor:GetComponentByKey(key) obtains reference to a component via key
    luabridge::LuaRef GetComponentByKey(const std::string &key)
    {
        auto it = components.find(key);
        if (it != components.end())
        {
            return *it->second;
        }

        return {LuaManager::lua_state}; // if the key doesn't exist
    }

    // self.actor:GetComponent(type) obtains reference to a component via type
    // If multiple components of the same type exist, the first one found is returned
    luabridge::LuaRef GetComponentByType(const std::string &type)
    {
        for (auto &component: components)
        {
            auto component_type = (*component.second)["type"].cast<std::string>();

            if (component_type == type && !(*component.second)["removed"].cast<bool>())
            {
                return *component.second;
            }

            if (component_type == type && (*component.second)["removed"].cast<bool>())
            {
                return {LuaManager::GetLuaState()};
            }

        }

        return {LuaManager::lua_state}; // if the type doesn't exist
    }

    // self.actor:GetComponents(type_name) obtains reference to all components of type
    // return in the form of an indexed table that may be iterated through with ipairs()
    // remember that lua tables index starting at 1 and not 0
    // return an empty table if no components of the desired type exist
    // LuaBridge cannot auto-convert std::vector to a table. Create a table manually.
    luabridge::LuaRef *GetComponents(const std::string &type)
    {
        auto *table = new luabridge::LuaRef(LuaManager::lua_state);

        int index = 1;

        for (auto &component: components)
        {
            luabridge::LuaRef *component_ref = component.second;
            auto component_type = (*component_ref)["type"].cast<std::string>();
            if (component_type == type)
            {
                // Add the component to the Lua table
                (*table)[index] = component_ref;
                index++;
            }
        }

        return table; // Return the table filled with components
    }

//    Collect then Alter: Mark Removal -> Actual Removal
//    self.actor:RemoveComponent(component_ref) Remove component from actor.
//    Immediately set the component’s enabled variable to false.
//    The removed component should not execute any more lifecycle functions after this call.
//    Warning : Be careful not to alter a container (remove from map / set) while iterating over it.
//    Consider using the Collect-Then-Alter pattern.
    void RemoveComponent(const luabridge::LuaRef &component_ref)
    {
        std::string key = component_ref["key"].tostring();
        component_ref["enabled"] = false; // Immediately set the component’s enabled variable to false
        component_ref["removed"] = true; // Immediately set the component’s enabled variable to false
        componentsToRemove.push_back(key);

        // When a Rigidbody is removed, its Box2D body will be unregistered
        // with the physics world and stop simulating immediately
        // FIXME: the way to get body from component_ref["body"] may be wrong
        if (component_ref["type"].tostring() == "Rigidbody")
        {
            b2Body *body = component_ref["body"].cast<b2Body *>();
            if (body)
            {
                world->DestroyBody(body);
            }
        }
    }

//    Destroy an actor, removing it and all of its components from the scene.
//    No lifecycle functions on the actor’s components should run after this function call.
//    The actual destruction of the actor should occur at the end of the current frame
//    (after all OnLateUpdates would have been called).
    void OnDestroy()
    {
        components.clear();
    }


    void OnTriggerEnter(const Collision &collision)
    {
        if (componentsOnTriggerEnter.empty())
        {
            for (auto &component: components)
            {
                if ((*component.second)["OnTriggerEnter"].isFunction())
                {
                    componentsOnTriggerEnter.insert({component.first, component.second});
                    (*component.second)["OnTriggerEnter"](*component.second, collision);
                }
            }
        }
        else
        {
            for (auto &component: componentsOnTriggerEnter)
            {
                if ((*component.second)["OnTriggerEnter"].isFunction())
                {
                    (*component.second)["OnTriggerEnter"](*component.second, collision);
                }
            }
        }
    }

    void OnTriggerExit(const Collision &collision)
    {
        if (componentsOnTriggerExit.empty())
        {
            for (auto &component: components)
            {
                if ((*component.second)["OnTriggerExit"].isFunction())
                {
                    componentsOnTriggerExit.insert({component.first, component.second});
                    (*component.second)["OnTriggerExit"](*component.second, collision);
                }
            }
        }
        else
        {
            for (auto &component: componentsOnTriggerExit)
            {
                if ((*component.second)["OnTriggerExit"].isFunction())
                {
                    (*component.second)["OnTriggerExit"](*component.second, collision);
                }
            }
        }

    }

    void OnCollisionEnter(const Collision &collision)
    {
        if (componentsOnCollisionEnter.empty())
        {
            for (auto &component: components)
            {
                if (((*component.second)["OnCollisionEnter"]).isFunction())
                {
                    componentsOnCollisionEnter.insert({component.first, component.second});
                    (*component.second)["OnCollisionEnter"](*component.second, collision);
                }
            }
        }
        else
        {
            for (auto &component: componentsOnCollisionEnter)
            {
                if ((*component.second)["OnCollisionEnter"].isFunction())
                {
                    (*component.second)["OnCollisionEnter"](*component.second, collision);
                }
            }
        };

    }


    void OnCollisionExit(const Collision &collision)
    {
        if (componentsOnCollisionExit.empty())
        {
            for (auto &component: components)
            {
                if ((*component.second)["OnCollisionExit"].isFunction())
                {
                    componentsOnCollisionExit.insert({component.first, component.second});
                    (*component.second)["OnCollisionExit"](*component.second, collision);
                }
            }
        }
        else
        {
            for (auto &component: componentsOnCollisionExit)
            {
                if ((*component.second)["OnCollisionExit"].isFunction())
                {
                    (*component.second)["OnCollisionExit"](*component.second, collision);
                }
            }
        }
    }

    void OnStart()
    {
        for (const auto &component: componentsOnStart)
        {
            // only if those actors come from the current scene but not the other scene
            // only if it has the OnStart function
            luabridge::LuaRef onStart = (*component.second)["OnStart"];
            if (onStart.isFunction() && (*component.second)["enabled"] && !(*component.second)["removed"])
            {
                try
                {
                    onStart(*component.second);
                }
                catch (const luabridge::LuaException &e)
                {
                    std::cout << "\033[31m" << name << " : " << e.what()
                              << "\033[0m" << std::endl;
                }

            }
        }
    }

    void OnUpdate()
    {
        for (const auto &component: componentsOnUpdate)
        {
            // only if those actors come from the current scene but not the other scene
            // only if it has the OnStart function
            luabridge::LuaRef onUpdate = (*component.second)["OnUpdate"];
            if (onUpdate.isFunction() && (*component.second)["enabled"] && !(*component.second)["removed"])
            {
                try
                {
                    onUpdate(*component.second);
                }
                catch (const luabridge::LuaException &e)
                {
                    std::cout << "\033[31m" << name << " : " << e.what()
                              << "\033[0m" << std::endl;
                }
            }
        }
    }


};

// Collision Detection class in Box2D
class CollisionDetector : public b2ContactListener
{
public:
    void EndContact(b2Contact *contact) override;

    void BeginContact(b2Contact *contact) override;
};


#endif // MAIN_CPP_ACTOR_H
