//
// Created by Jhin Pan on 3/27/24.
//

#ifndef MAIN_CPP_RAYCAST_H
#define MAIN_CPP_RAYCAST_H

#include "box2d.h"
#include "Actor.h"
#include "Rigidbody.h"
#include "LuaMananger.h"
#include <vector>
#include <algorithm>

// Define the HitResult to store raycast hit information
struct HitResult
{
    Actor *actor; // the actor that our raycast found
    b2Vec2 point; // the point at which the raycast struck a fixture of the actor
    b2Vec2 normal; // the normal vector at the point
    bool is_trigger; // whether the fixture encountered is a trigger (sensor)
    float fraction; // the fraction along the ray at the point of intersection

    // Constructor
    HitResult(Actor *actor, b2Vec2 point, b2Vec2 normal, bool is_trigger, float fraciton) :
            actor(actor), point(point), normal(normal), is_trigger(is_trigger), fraction(fraciton) {}
};

class RayCastCallback : public b2RayCastCallback
{
public:
    std::vector<HitResult> hits; // store all the hits

    // This function is called for each fixture found in the query.
    // Return the fraction of the ray for the closest hit
    float ReportFixture(b2Fixture *fixture, const b2Vec2 &point, const b2Vec2 &normal, float fraction) override
    {
        auto *actor = reinterpret_cast<Actor *>(fixture->GetUserData().pointer);

        if (actor == nullptr)
        {
            // Phantom fixture detected. Ignore it.
            return -1.0f;
        }

        bool is_trigger = fixture->IsSensor();
        hits.emplace_back(actor, point, normal, is_trigger, fraction);

        // Returning fraction continues the raycast to find closer hits
        return 1.0f;
    }

    // Method to clear hits
    void ClearHits()
    {
        hits.clear();
    }
};

class Physics
{
public:

    // Perform a Raycast with Box2D and return the first actor hit on the path of the ray.
    static luabridge::LuaRef Raycast(b2Vec2 pos, b2Vec2 dir, float dist)
    {
        // if the distance is 0/negative or there are no rigidbodies in existence
        if (!world || dist <= 0) return {LuaManager::lua_state}; // return nil

        RayCastCallback callback;

        dir.Normalize(); // Normalize the direction vector to avoid one-pixel issue

        b2Vec2 endPos = pos + dist * dir; // calculate the end position of the raycast

        world->RayCast(&callback, pos, endPos); // perform the raycast

        float closestFraction = 1; // Find the first actor hit on the path of the ray

        HitResult *closestHit = nullptr;

        for (auto &hit: callback.hits)
        {
            // if (hit.is_trigger) continue; // skip triggers

            if (closestHit == nullptr || hit.fraction < closestFraction)
            {
                closestHit = &hit;
                closestFraction = hit.fraction;
            }
        }

        if (closestHit != nullptr)
        {
            luabridge::LuaRef componentRef(LuaManager::lua_state, *closestHit);

            return componentRef; // Return the table wrapped in a LuaRef.
        }
        else
        {
            // The raycast fails to hit anything
            return {LuaManager::lua_state}; // return nil
        }

    }

    // Return all hits(fixtures) that occur during the raycast operation,
    // sorted by distance along the raycast(nearest to furthest)
    static luabridge::LuaRef RaycastAll(const b2Vec2 &pos, const b2Vec2 &dir, float dist)
    {
        if (!world || dist <= 0) return {LuaManager::lua_state}; // return an empty vector

        RayCastCallback callback;

        b2Vec2 endPos = pos + dist * dir; // Not Normalized for now

        world->RayCast(&callback, pos, endPos); // perform the raycast

        // Sort the hits by distance along the ray
        std::sort(callback.hits.begin(), callback.hits.end(), [](const HitResult &a, const HitResult &b)
        {
            return a.fraction < b.fraction;
        });

        // Create a new Lua table to hold the hit results
        luabridge::LuaRef hitResultsTable = luabridge::newTable(LuaManager::lua_state);

        // Fill the Lua table with the hit results
        for (size_t i = 0; i < callback.hits.size(); ++i)
        {
            auto hit = callback.hits[i];
            // Lua is 1-indexed
            hitResultsTable[i + 1] = hit;
        }

        return hitResultsTable; // Return the table filled with hit results
    }
};


#endif //MAIN_CPP_RAYCAST_H

