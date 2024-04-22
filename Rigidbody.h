//
// Created by Jhin Pan on 3/21/24.
//

#ifndef MAIN_CPP_RIGIDBODY_H
#define MAIN_CPP_RIGIDBODY_H

#include <iostream>
#include <string>
#include <cmath>
// #include "Actor.h"
#include "box2d.h"
#include "Helper.h"

// If we want to make rigidbody as a C++ component, we need to:
// 1. Create a class that has variables a standard component has
// 2. Inject a C++ pointer into a Luabridge object
// 3. Store that LuaRef as a component on an actor

class Actor; // forward declaration

class Rigidbody
{
public:
    // should the normal variable for one rigidbody be static inline as well?
    float x = 0.0f; // Use b2BodyDef.position (in C++)
    float y = 0.0f;

    float width = 1.0f; // Use b2PolygonShape.SetAsBox (in C++)
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;

    b2Body *body = nullptr;

    std::string bodyType = "dynamic"; // Use b2BodyDef.type (in C++)

    bool removed = false; // cause we are using the old component system

    bool precise = true; // Use b2BodyDef.bullet
    float gravity_scale = 1.0f; // Use b2BodyDef.gravityScale
    float density = 1.0f; // set density on a fixture
    float angular_friction = 0.3f; // Use b2BodyDef.angularDamping

    // the Lua-coder only ever deals with/sees clockwise degrees
    float rotation = 0.0f; // Units of degrees (not radians)
    // We will need to convert, as Box2D uses radians
    // float radians = rotation_degrees * (b2_pi / 180.0f);
    // float degrees = rotation_radians * (180.0f / b2_pi);

    bool has_collider = true; // create a non-sensor fixture if true
    std::string collider_type = "box"; // define the shape of the collider
    // box: use b2PolygonShape and .SetAsBox() when creating a fixture
    // circle: use b2CircleShape and .m_radius when creating a fixture
    bool has_trigger = true; // create a sensor fixture if true
    std::string trigger_type = "box"; // define the shape of the trigger
    float trigger_width = 1.0f; // use b2PolygonShape and .SetAsBox() when creating a fixture
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f; // use b2CircleShape and .m_radius when creating a fixture

    // To ensure it will be seen as a C++ component:
    std::string componentType = "Rigidbody";
    std::string key = "???";
    Actor *actor = nullptr;
    bool enabled = true;

    const uint16 BOX2D_CATEGORY_COLLIDER = 0x0001;
    const uint16 BOX2D_CATEGORY_SENSOR = 0x0002;

    b2Vec2 GetBodyPosition() const
    {
        // x and y govern "where" a body is considered to be prior
        // to OnStart(). Afterwards we get that info from the body directly
        if (body == nullptr)
            return {x, y};
        return body->GetPosition();
    }

    float GetBodyRotation() const
    {
        if (body == nullptr)
            return rotation;
        else
        {
            return body->GetAngle() * (180.0f / b2_pi);
        }
    }

    /// to alter an associated b2Body -
    void AddForce(const b2Vec2 &force) const
    {
        body->ApplyForceToCenter(force, true);
    }

    void SetVelocity(const b2Vec2 &velocity) const
    {
        body->SetLinearVelocity(velocity);
    }

    void SetPosition(const b2Vec2 &position)
    {
        if (body == nullptr)
        {
            x = position.x;
            y = position.y;
        }
        else
        {
            body->SetTransform(position, body->GetAngle());
        }
    }

    void SetRotation(float degrees_clockwise)
    {
        if (body == nullptr)
        {
            rotation = degrees_clockwise;
        }
        else
        {
            float radians = degrees_clockwise * (b2_pi / 180.0f);
            body->SetTransform(body->GetPosition(), radians);
        }
    }

    void SetAngularVelocity(float degrees_clockwise)
    {
        float radians = degrees_clockwise * (b2_pi / 180.0f);
        body->SetAngularVelocity(radians);
    }

    void SetGravityScale(float scale) const
    {
        body->SetGravityScale(scale);
    }

    // This performs a rotation of the rigidbody such that its local "up" vector
    // (the vector that points out the top of the body, even as it rotates) points in a certain direction.
    // Use glm::atan(x, -y) to obtain an angle from a vector(in radians)
    // and don't forget to normalize the direction parameter
    void SetUpDirection(const b2Vec2 &direction)
    {
        b2Vec2 normalized_direction = direction;
        normalized_direction.Normalize();
        float new_angle_radians = glm::atan(normalized_direction.x, -normalized_direction.y);
        body->SetTransform(body->GetPosition(), new_angle_radians);
    }

    // This performs a rotation of the rigidbody such that its local "right" vector
    // (the vector that points out the right side of the body, even as it rotates) points in a certain direction.
    void SetRightDirection(const b2Vec2 &direction)
    {
        b2Vec2 normalized_direction = direction;
        normalized_direction.Normalize();
        float new_angle_radians = glm::atan(normalized_direction.x, -normalized_direction.y) - (b2_pi / 2.0f);
        body->SetTransform(body->GetPosition(), new_angle_radians);
    }

    /// functions to check an associated b2Body -
    // use b2Body::GetLinearVelocity() to get the velocity of the body
    b2Vec2 GetVelocity() const
    {
        return body->GetLinearVelocity();
    }

    // use b2Body::GetAngularVelocity() to get the angular velocity of the body
    float GetAngularVelocity() const
    {
        // std::cout << "GetAngularVelocity called" << std::endl;
        // std::cout << "angular velocity got as: " << body->GetAngularVelocity() * (180.0f / b2_pi) << std::endl;
        return body->GetAngularVelocity() * (180.0f / b2_pi);
    }

    // use b2Body::GetGravityScale() to get the gravity scale of the body
    float GetGravityScale() const
    {
        return body->GetGravityScale();
    }

    // return the normalized "up" vector for this body
    // This is the vector that points locally "up" out of a body, changing as it rotates
    b2Vec2 GetUpDirection() const
    {
        float angle = body->GetAngle();
        b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
        result.Normalize();
        return result;
    }

    // return the normalized "right" vector for this body
    // This is the vector that points locally "right" out of a body, changing as it rotates
    b2Vec2 GetRightDirection() const
    {
        float angle = body->GetAngle();
        b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
        result.Normalize();
        return result;
    }

    // Ready Function: Kind of OnStart() for Rigidbody
    void Ready()
    {
        // so that we only initialize the world once
        if (!world_initialized)
        {
            world = new b2World(b2Vec2(0.0f, 9.8f));

            auto *detector = new CollisionDetector();  //  detector = contact_listener
            world->SetContactListener(detector);

            world_initialized = true;
        }

        b2BodyDef bodyDef;
        if (bodyType == "dynamic")
        {
            bodyDef.type = b2_dynamicBody;
        }
        else if (bodyType == "static")
        {
            bodyDef.type = b2_staticBody;
        }
        else if (bodyType == "kinematic")
        {
            bodyDef.type = b2_kinematicBody;
        }

        bodyDef.position.Set(x, y);
        // set the rotation to the bodyDef
        float rotationRadians = rotation * (b2_pi / 180.0f); // degree to radians
        bodyDef.angle = rotationRadians; // Set the rotation in radians to the bodyDef
        bodyDef.bullet = precise;
        bodyDef.angularDamping = angular_friction;
        bodyDef.gravityScale = gravity_scale;

        body = world->CreateBody(&bodyDef); // create the body
        // std::cout << "Rigidbody registered in " << Helper::GetFrameNumber() << std::endl;

        // Create Collider Fixture
        if (has_collider)
        {
            b2Shape *colliderShape = nullptr;
            if (collider_type == "box")
            {
                auto *polygonShape = new b2PolygonShape();
                polygonShape->SetAsBox(width * 0.5f, height * 0.5f); // 1.0f x 1.0f box
                colliderShape = polygonShape;
            }
            else if (collider_type == "circle")
            {
                auto *circleShape = new b2CircleShape();
                circleShape->m_radius = radius;
                colliderShape = circleShape;
            }

            b2FixtureDef colliderFixtureDef;
            colliderFixtureDef.isSensor = false;
            colliderFixtureDef.filter.categoryBits = BOX2D_CATEGORY_COLLIDER;
            colliderFixtureDef.filter.maskBits = ~BOX2D_CATEGORY_SENSOR;
            colliderFixtureDef.shape = colliderShape;
            colliderFixtureDef.density = density;
            colliderFixtureDef.friction = friction;
            colliderFixtureDef.restitution = bounciness;
            colliderFixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(actor);

            body->CreateFixture(&colliderFixtureDef); // create the fixture
        }

        if (has_trigger)
        {
            b2Shape *triggerShape = nullptr;
            if (trigger_type == "box")
            {
                auto *polygonShape = new b2PolygonShape();
                polygonShape->SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f); // 1.0f x 1.0f box
                triggerShape = polygonShape;
            }
            else if (trigger_type == "circle")
            {
                auto *circleShape = new b2CircleShape();
                circleShape->m_radius = trigger_radius;
                triggerShape = circleShape;
            }

            b2FixtureDef triggerFixtureDef;
            triggerFixtureDef.isSensor = true;
            triggerFixtureDef.filter.categoryBits = BOX2D_CATEGORY_SENSOR;
            triggerFixtureDef.filter.maskBits = ~BOX2D_CATEGORY_COLLIDER;
            triggerFixtureDef.shape = triggerShape;
            triggerFixtureDef.density = density;
            triggerFixtureDef.friction = friction;
            triggerFixtureDef.restitution = bounciness;
            triggerFixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(actor);

            body->CreateFixture(&triggerFixtureDef); // create the fixture
        }

        // phantom sensor to make bodies move if neither collider nor trigger is present
        if (!has_collider && !has_trigger)
        {
            b2PolygonShape phantomShape;
            phantomShape.SetAsBox(width * 0.5f, height * 0.5f); // 1.0f x 1.0f box

            b2FixtureDef phantomFixtureDef;
            phantomFixtureDef.shape = &phantomShape;
            phantomFixtureDef.density = density;
            phantomFixtureDef.friction = angular_friction;

            // Because it is a sensor (with no callback even), no collisions will ever occur
            phantomFixtureDef.isSensor = true;
            body->CreateFixture(&phantomFixtureDef); // create the fixture
        }

    };


};


#endif //MAIN_CPP_RIGIDBODY_H
