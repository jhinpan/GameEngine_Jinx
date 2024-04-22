#ifndef MAIN_CPP_SCENE_H
#define MAIN_CPP_SCENE_H

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include "AudioManager.h"
#include "ComponentManager.h"
#include "Helper.h"
#include "Input.h"
#include "Actor.h"
#include "Renderer.h"
#include "EngineUtils.h"
#include "rapidjson/document.h"
#include "glm/glm.hpp"
#include "LuaMananger.h"

class Scene
{
public:
    static inline std::vector<Actor *> actors; // List of actors in the scene

    glm::vec2 cameraPosition{}; // Camera position - default constructor sets to (0, 0)
    std::string nextSceneName; // Name of the next scene to proceed to

    bool gameEnd = false, gameWin = false, sceneChange = false; // Flags to indicate game win and scene change

    AudioManager sceneAudioManager; // Audio manager for the scene
    ComponentManager componentManager; // Component manager for the scene

    Scene() = default; // Default constructor

    // Add a actor to the scene
    static void addActor(Actor *actor)
    {
        actors.push_back(actor); // Add pointer to vector
    }

    // Get access to actors (read-only for now)
    static std::vector<Actor *> &getActors()
    {
        return actors;
    }

    static void
    ApplyComponentOverrides(lua_State *L, const std::string &componentName, const rapidjson::Value &componentValue)
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

    void updateFromTemplate(Actor *actor, const std::string &templatePath)
    {
        rapidjson::Document document;
        EngineUtils::ReadJsonFile(templatePath, document);

        // Temporary map for components
        std::map<std::string, luabridge::LuaRef *> tempComponents;

        if (document.HasMember("components") && document["components"].IsObject())
        {
            const auto &componentsObject = document["components"].GetObject();
            for (const auto &component: componentsObject)
            {
                const std::string componentName = component.name.GetString();
                const std::string componentType = component.value["type"].GetString();

                std::string componentPath = "resources/component_types/" + componentType + ".lua";

                // Check if the component Lua file exists
                if (!std::filesystem::exists(componentPath))
                {
                    std::cout << "error: failed to locate component " << componentType;
                    exit(0);
                }

                // Load and instance the component
                componentManager.InstantiateComponent(componentName, componentType, actor);

                tempComponents.insert(
                        {componentName, ComponentManager::component_tables[componentName]});

                // Apply overrides
                ApplyComponentOverrides(LuaManager::lua_state, componentName, component.value);
            }

            actor->components = std::move(tempComponents);
        }


    }

    // Load scene data (all actors' data) from a .scene file
    void LoadFromJson(const std::string &file_path)
    {
        rapidjson::Document document;
        EngineUtils::ReadJsonFile(file_path, document);

        if (document.HasMember("actors") && document["actors"].IsArray())
        {
            const auto &actorsArray = document["actors"].GetArray();
            for (const auto &actorValue: actorsArray)
            {
                if (actorValue.IsObject())
                {
                    auto *actor = new Actor();

                    actor->updateFromJson(actorValue); // only update the actor's name for now

                    if (actorValue.HasMember("template") && actorValue["template"].IsString())
                    {

                        const std::string templateName = actorValue["template"].GetString();
                        const std::string templatePath = "resources/actor_templates/" + templateName + ".template";
                        // new function to load actor template
                        updateFromTemplate(actor, templatePath);


                        if (actorValue.HasMember("components") && actorValue["components"].IsObject())
                        {
                            // Temporary map for components
                            std::map<std::string, luabridge::LuaRef *> tempComponents;

                            const auto &componentsObject = actorValue["components"].GetObject();

                            for (const auto &component: componentsObject)
                            {
                                // for things like test-case 1-5
                                if (component.value.HasMember("type") && component.value["type"].IsString())
                                {

                                    const std::string componentName = component.name.GetString();
                                    const std::string componentType = component.value["type"].GetString();

                                    std::string componentPath = "resources/component_types/" + componentType + ".lua";

                                    // Check if the component Lua file exists
                                    if (!std::filesystem::exists(componentPath))
                                    {
                                        std::cout << "error: failed to locate component " << componentType;
                                        exit(0);
                                    }

                                    // Load and instance the component
                                    componentManager.InstantiateComponent(componentName, componentType, actor);

                                    tempComponents.insert(
                                            {componentName, ComponentManager::component_tables[componentName]});

                                    // Apply overrides
                                    ApplyComponentOverrides(LuaManager::lua_state, componentName, component.value);
                                }
                                else // for things like test-case 1-6
                                {
                                    const std::string componentName = component.name.GetString();
                                    tempComponents.insert(
                                            {componentName, ComponentManager::component_tables[componentName]});
                                    // Apply overrides
                                    ApplyComponentOverrides(LuaManager::lua_state, componentName, component.value);
                                }

                            }

                            actor->components = std::move(tempComponents);

                        }


                    }

                    if (actorValue.HasMember("components") && actorValue["components"].IsObject() &&
                        !actorValue.HasMember("template"))
                    {
                        // Temporary map for components
                        std::map<std::string, luabridge::LuaRef *> tempComponents;

                        const auto &componentsObject = actorValue["components"].GetObject();
                        for (const auto &component: componentsObject)
                        {
                            const std::string componentName = component.name.GetString();
                            const std::string componentType = component.value["type"].GetString();

                            if (componentType != "Rigidbody")
                            {
                                std::string componentPath = "resources/component_types/" + componentType + ".lua";

                                // Check if the component Lua file exists
                                if (!std::filesystem::exists(componentPath))
                                {
                                    std::cout << "error: failed to locate component " << componentType;
                                    exit(0);
                                }

                                // Load and instance the component
                                componentManager.InstantiateComponent(componentName, componentType, actor);

                                tempComponents.insert(
                                        {componentName, ComponentManager::component_tables[componentName]});

                                // Apply overrides
                                ApplyComponentOverrides(LuaManager::lua_state, componentName, component.value);
                            }
                            else // for Rigidbody, this kind of C++ component
                            {
                                auto *rbComponent = ComponentManager::CreateRigidbody(componentName, actor);
                                // rbComponent->componentType = "Rigidbody";
                                // rbComponent = table["rigidbody"] = componentRef

                                // and even the template for Rigidbody?

                                // for property in ComponentJson:
                                //     AddPropertyToComponent(component, property)
                                ApplyComponentOverrides(LuaManager::lua_state, componentName, component.value);
                                // need to rewrite the component overrides

                                // add the Ready function to the Rigidbody class
                                if ((*rbComponent)["Ready"].isFunction())
                                {
                                    (*rbComponent)["Ready"](*rbComponent);
                                    // std::cout << "Ready function called for Rigidbody" << std::endl;
                                }

                                tempComponents.insert({componentName, rbComponent});

                            }

                        }

                        actor->components = std::move(tempComponents);
                    }


                    addActor(actor);
                    ComponentManager::actorTable.emplace_back(actor); // differ push_back and emplace_back again
                }
            }
        }
    }

    // FIXME: Need to fix the LoadScene function
    void LoadNextScene(const std::string &sceneName)
    {
        std::string scenePath = "resources/scenes/" + sceneName + ".scene";

        // Clear the current scene data Or DontDestroyOnLoad
        // actors.clear();

        // Handle Don't Destroy On Load in Scene Loading
        // for (int i = 0; i < Scene::currentScene.actors.size(); i++)
        // {
        //     if (Scene::currentScene.actors[i].dontDestroyOnLoad)
        //     {
        //         scene.actors.push_back(Scene::currentScene.actors[i]);
        //         Scene::currentScene.actors[i]->fromAnotherScene = true;
        //     }
        // }

        // if (actor->dontDestroyOnLoad) we keep it in the actors as well and label it as fromAnotherScene
        for (auto &actor: actors)
        {
            if (actor->dontDestroyOnLoad)
            {
                actor->fromAnotherScene = true;
            }
            else
            {
                // erase it from static inline std::vector<Actor *> actors; // List of actors in the scene
                actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());

                // remove it from actorTable as well:
                // ComponentManager::actorTable.erase(std::remove(ComponentManager::actorTable.begin(), ComponentManager::actorTable.end(), actor), ComponentManager::actorTable.end());
            }
        }

        // std::cout << "Actors size for now: " << actors.size() << std::endl;

        // std::cout << "Loading Scene: " << sceneName << std::endl;

        // Load the new scene
        LoadFromJson(scenePath);

        // std::cout << "Actors size after loading: " << actors.size() << std::endl;
    }

};


#endif // MAIN_CPP_SCENE_H

