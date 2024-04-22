//
// Created by Jhin Pan on 3/29/24.
//

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "LuaMananger.h"
#include "box2d.h"
#include "Actor.h"
#include "Rigidbody.h"

// BeginContact is called when two fixtures begin to overlap/touch
void CollisionDetector::BeginContact(b2Contact *contact)
{
    // get fixture references via contact->GetFixture()
    b2Fixture *fixtureA = contact->GetFixtureA();
    b2Fixture *fixtureB = contact->GetFixtureB();

    auto *actorA = reinterpret_cast<Actor *>(fixtureA->GetUserData().pointer);
    auto *actorB = reinterpret_cast<Actor *>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB)
        return;

    b2WorldManifold worldManifold;
    contact->GetWorldManifold(&worldManifold);

    Actor::Collision collisionInfo;
    collisionInfo.other = actorB;
    collisionInfo.point = worldManifold.points[0]; // First contact point?
    collisionInfo.relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
    // The normal vector points from fixtureA to fixtureB or fixtureB to fixtureA?
    collisionInfo.normal = worldManifold.normal;

    // Determine this is a trigger or collision event
    if (fixtureA->IsSensor() && fixtureB->IsSensor())
    {
        collisionInfo.point = b2Vec2(-999.0f, -999.0f); // No contact point
        collisionInfo.normal = b2Vec2(-999.0f, -999.0f); // Sen normal value

        actorA->OnTriggerEnter(collisionInfo);

        collisionInfo.other = actorA;
        actorB->OnTriggerEnter(collisionInfo);
    }
    else if (!fixtureA->IsSensor() && !fixtureB->IsSensor())
    {
        actorA->OnCollisionEnter(collisionInfo);

        collisionInfo.other = actorA;
        actorB->OnCollisionEnter(collisionInfo);
    }

}

void CollisionDetector::EndContact(b2Contact *contact)
{
    // get fixture references via contact->GetFixture()
    b2Fixture *fixtureA = contact->GetFixtureA();
    b2Fixture *fixtureB = contact->GetFixtureB();

    auto *actorA = reinterpret_cast<Actor *>(fixtureA->GetUserData().pointer);
    auto *actorB = reinterpret_cast<Actor *>(fixtureB->GetUserData().pointer);

    if (!actorA || !actorB)
        return;

    Actor::Collision collisionInfo;
    collisionInfo.other = actorB;
    collisionInfo.point = b2Vec2(-999.0f, -999.0f); // No contact point
    collisionInfo.relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
    // FIXME: do we still need to have relative_velocity calculated or just empty?
    collisionInfo.normal = b2Vec2(-999.0f, -999.0f); // Sen normal value

    if (fixtureA->IsSensor() && fixtureB->IsSensor())
    {
        actorA->OnTriggerExit(collisionInfo);

        collisionInfo.other = actorA;
        actorB->OnTriggerExit(collisionInfo);
    }
    else if (!fixtureA->IsSensor() && !fixtureB->IsSensor())
    {
        actorA->OnCollisionExit(collisionInfo);

        collisionInfo.other = actorA;
        actorB->OnCollisionExit(collisionInfo);
    }

}

//    self.actor:AddComponent(type_name) : Add component to actor and return reference to it.
//    The new component should begin executing lifecycle functions on the next frame.
//    The component’s “key” (which remember, determines execution order) has a formula–
//    “rn” where “r” stands for runtime-added and “n” is the number of times AddComponent(type_name) has been called in the entire program (“n” is a global integer counter, not one local to the actor).
//    Warning : Be careful not to alter a container (add to map / set) while iterating over it.
//    Consider using the Collect-Then-Alter pattern.
// Currently, dont realize the feature of override
luabridge::LuaRef *Actor::AddComponent(const std::string &type)
{
    std::string key = "r" + std::to_string(component_id++); // Generate the key

    const std::string path = "resources/component_types/" + type + ".lua";

    if (type != "Rigidbody")
    {
        if (luaL_dofile(LuaManager::lua_state, path.c_str()) != LUA_OK)
        {
            std::cout << "problem with lua file " << type;
            exit(0);
        }

        // auto here is Luabridge::LuaRef
        auto *base_table = new luabridge::LuaRef(luabridge::getGlobal(LuaManager::lua_state, type.c_str()));

        // auto here is Luabridge::LuaRef
        // Create a new empty table to represent the new component instance
        auto *component = new luabridge::LuaRef(luabridge::newTable(LuaManager::GetLuaState()));

        // We must create a metatable to establish inheritance in Lua
        luabridge::LuaRef new_metatable = luabridge::newTable(LuaManager::lua_state);
        new_metatable["__index"] = *base_table;

        // use the raw lua C-API (lua stack) to perform a "setmetatable" operation
        (*component).push(LuaManager::lua_state);
        new_metatable.push(LuaManager::lua_state);
        lua_setmetatable(LuaManager::lua_state, -2);
        lua_pop(LuaManager::lua_state, 1);

        (*component)["type"] = type; // Set the type of the component
        (*component)["key"] = key; // Set the key of the component
        (*component)["enabled"] = true;
        (*component)["actor"] = this;

        Actor::componentsAdded[key] = component; // Add the component to the actor's components map

        if ((*component)["OnStart"])
        {
            componentsAwaitingOnStart.push_back(component);
        }

        return component; // Return the reference to the component
    }
    else
    {
        // How to add a Rigidbody at runtime
        // As we include Actor in the Rigidbody.h, but now we need to use
        // auto *rigidbody = new Rigidbody();
        auto *rigidbody = new Rigidbody();

        luabridge::push(LuaManager::lua_state, rigidbody);

        // Rigidbody*
        auto *componentRef = new luabridge::LuaRef(luabridge::LuaRef::fromStack(LuaManager::lua_state, -1));

        (*componentRef)["actor"] = this;
        (*componentRef)["type"] = type;
        (*componentRef)["key"] = key;
        (*componentRef)["enabled"] = true;
        (*componentRef)["removed"] = false;

        Actor::componentsAdded[key] = componentRef;

        // OnStart at the start of the next frame

        if ((*componentRef)["Ready"].isFunction())
        {
            componentsOnReady.emplace_back(componentRef);
        }

        return componentRef;
    }


}
