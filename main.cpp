#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include "rapidjson/document.h"
#include "EngineUtils.h"
#include "Scene.h"
#include "Helper.h"
#include "Renderer.h"
#include "AudioManager.h"
#include "Input.h"
#include "SDL2/SDL.h"
#include "SDL2_image/SDL_image.h"
#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "box2d.h"
#include "emscripten.h"

enum GameState
{
    INTRO,
    SCENE,
    ENDING
};

GameState gameState = SCENE;
bool quit = false; // Main loop flag
Renderer renderer;
Scene currentScene;

void initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        exit(1);
    }
    if (TTF_Init() == -1)
    {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: " << TTF_GetError() << std::endl;
        exit(1);
    }

    // After SDL, IMG, and TTF initialization, initialize the audio system
    AudioManager audioManager;
    std::string gameTitle; // Default to empty string if not defined


    if (!std::filesystem::exists("resources/game.config"))
    {
        std::cerr << "error: resources/game.config missing";
        exit(1);
    }
    rapidjson::Document gameConfig;
    EngineUtils::ReadJsonFile("resources/game.config", gameConfig);
    // load game title and intro images from game.config
    if (gameConfig.HasMember("game_title") && gameConfig["game_title"].IsString())
        gameTitle = gameConfig["game_title"].GetString();


    // Resolution settings
    int windowWidth = 640; // Default width
    int windowHeight = 360; // Default height
    rapidjson::Document renderingConfig;
    if (std::filesystem::exists("resources/rendering.config"))
    {
        EngineUtils::ReadJsonFile("resources/rendering.config", renderingConfig);
        if (renderingConfig.HasMember("x_resolution"))
            windowWidth = renderingConfig["x_resolution"].GetInt();
        if (renderingConfig.HasMember("y_resolution"))
            windowHeight = renderingConfig["y_resolution"].GetInt();
    }

    renderer.Initialize(gameTitle, windowWidth, windowHeight);

    std::string scenePath = "resources/scenes/";
    if (gameConfig.HasMember("initial_scene") && gameConfig["initial_scene"].IsString())
    {
        scenePath += gameConfig["initial_scene"].GetString();
        LuaManager::currentSceneName = gameConfig["initial_scene"].GetString(); // a bit silly here
        scenePath += ".scene";
        currentScene.LoadFromJson(scenePath);

        // After loading the scene, invoke OnStart for all components
        // Initialize components
        for (auto actor: Scene::getActors())
        {
            if (!actor->fromAnotherScene)
            {
                // Queue OnStart calls for each component in alphabetical order
                if (actor->componentsOnStart.empty())
                {
                    for (const auto &component: actor->components)
                    {
                        // only if those actors come from the current scene but not the other scene
                        // only if it has the OnStart function
                        luabridge::LuaRef onStart = (*component.second)["OnStart"];
                        if (onStart.isFunction() && (*component.second)["enabled"] && !(*component.second)["removed"])
                        {
                            actor->componentsOnStart.insert({component.first, component.second});
                            try
                            {
                                onStart(*component.second);
                            }
                            catch (const luabridge::LuaException &e)
                            {
                                std::cout << "\033[31m" << actor->name << " : " << e.what()
                                          << "\033[0m" << std::endl;
                            }

                        }
                    }
                }
            }
        }

    }
}

void main_loop(){

    SDL_Event next_event; // Event handler
        if (quit) {
            emscripten_cancel_main_loop();  // Optionally stop the loop if needed
        }

    // Handle events(or actually inputs) on queue
    while (Helper::SDL_PollEvent498(&next_event))
    { // return 1 if events
        Input::ProcessEvent(next_event); // Call every frame at start of event loop.

        if (next_event.type == SDL_QUIT)
        {
            quit = true;
        }
    }

    // Clear Renderer: SDL_RenderClear(renderer)
    renderer.StartFrame();

    // Game State Handling
    switch (gameState)
    {
        case INTRO:
        {
            // break;
        }
        case SCENE:
        {
            // Run the newly added actors' OnStart functions
            // Queue OnStart calls for each component in alphabetical order
            if (!componentsAwaitingOnStart.empty())
            {
                for (const auto &component: componentsAwaitingOnStart)
                {

                    // only if it has the OnStart function
                    luabridge::LuaRef onStart = (*component)["OnStart"];
                    if (onStart.isFunction() && (*component)["enabled"] && !(*component)["removed"]
                        && !(*component)["actor"]["fromAnotherScene"])
                    {
                        try
                        {
                            onStart(*component);
                        }
                        catch (const luabridge::LuaException &e)
                        {
                            std::cout << "\033[31m" << " : " << e.what()
                                      << "\033[0m" << std::endl;
                        }
                    }

                }
                componentsAwaitingOnStart.clear();
            }

            // for actor in actors_to_add
            // actor.onStart()
            // actors.add(actor)
            for (auto actor: ComponentManager::actors_to_add)
            {
                actor->OnStart(); // using the componentsOnStart
                Scene::actors.push_back(actor);
            }
            ComponentManager::actors_to_add.clear();

            // Actually its supposed ot for actors_to_add since we are instantiating the actors with rigidbody to the scene
            // Consider a more time-saving way to handle with this
            for (auto actor: Scene::getActors())
            {
                for (const auto &component: actor->componentsOnReady)
                {

                    // only if it has the Ready function
                    luabridge::LuaRef ready = (*component)["Ready"];
                    if (ready.isFunction() && (*component)["enabled"] && !(*component)["removed"]
                        && !(*component)["actor"]["fromAnotherScene"])
                    {
                        try
                        {
                            ready(*component);
                        }
                        catch (const luabridge::LuaException &e)
                        {
                            std::cout << "\033[31m" << " : " << e.what()
                                      << "\033[0m" << std::endl;
                        }
                    }
                }
                actor->componentsOnReady.clear();
            }

            // add component in componentsAdded to the actor's current components
            // for actor in actors:
            // actor.ProcessAddedComponents()
            for (auto actor: Scene::getActors())
            {
                for (const auto &component: actor->componentsAdded)
                {
                    actor->components[component.first] = component.second;
                }
                actor->componentsAdded.clear();
            }

            // The behavior of actors will now be defined via components alone.
            // If a component contains an OnUpdate(self) function, call it every frame.
            // Make the call after we have finished calling OnStart for every actor and every component
            // calling order: Iterate through actors by ID, then components by key
            for (auto actor: Scene::getActors())
            {
                if (actor->componentsOnUpdate.empty())
                {
                    for (const auto &component: actor->components)
                    {
                        // only if it has the OnUpdate function
                        luabridge::LuaRef onUpdate = (*component.second)["OnUpdate"];
                        if (onUpdate.isFunction() && (*component.second)["enabled"] &&
                            !(*component.second)["removed"])
                        {
                            actor->componentsOnUpdate.insert({component.first, component.second});
                            try
                            {
                                onUpdate(*component.second);
                            }
                            catch (const luabridge::LuaException &e)
                            {
                                std::cout << "\033[31m" << actor->name << " : " << e.what()
                                          << "\033[0m" << std::endl;
                            }
                        }
                    }
                }
                else
                {
                    actor->OnUpdate(); // using the componentsOnUpdate
                }

            }

            // for actor in actors: actor.LateUpdate()
            for (auto actor: Scene::getActors())
            {
                for (const auto &component: actor->components)
                {
                    // only if it has the OnLateUpdate function
                    luabridge::LuaRef onLateUpdate = (*component.second)["OnLateUpdate"];
                    if (onLateUpdate.isFunction() && (*component.second)["enabled"] &&
                        !(*component.second)["removed"])
                    {
                        try
                        {
                            onLateUpdate(*component.second);
                        }
                        catch (const luabridge::LuaException &e)
                        {
                            std::cout << "\033[31m" << actor->name << " : " << e.what()
                                      << "\033[0m" << std::endl;
                        }

                    }

                    luabridge::LuaRef onDestroy = (*component.second)["OnDestroy"];
                    if (onDestroy.isFunction() &&
                        (*component.second)["removed"])
                    {
                        try
                        {
                            onDestroy(*component.second);
                        }
                        catch (const luabridge::LuaException &e)
                        {
                            std::cout << "\033[31m" << actor->name << " : " << e.what()
                                      << "\033[0m" << std::endl;
                        }

                    }
                }
            }




            // for actor in actors: actor.ProcessRemovedComponents()
            // Remove components in componentsToRemove from the actor's current components
            for (auto actor: Scene::getActors())
            {
                for (const auto &component: actor->componentsToRemove)
                {
                    // Check if the component exists in the actor's components map
                    auto it = actor->components.find(component);

                    // If found, remove the component from the map
                    if (it != actor->components.end())
                    {
                        actor->components.erase(it);
                    }
                }
                actor->componentsToRemove.clear();
            }

            // for actors in actors_to_destroy:
            // actor.OnDestroy()
            // actors.remove(actor)
            for (auto actor: ComponentManager::actors_to_remove)
            {
                actor->OnDestroy(); // clear all those components
                Scene::actors.erase(std::remove(Scene::actors.begin(), Scene::actors.end(), actor),
                                    Scene::actors.end());
                delete actor;
            }
            ComponentManager::actors_to_remove.clear();


            // EventBus.ProcessSubscriptions()
            EventBus::ProcessDeferredActions();

            // PhysicsStep()
//                b2Vec2 gravity(0.0f, 10.0f);
//                b2World world(gravity);

            if (world_initialized)
            {

                // std::cout << "PHYSICS STEP" << Helper::GetFrameNumber() << std::endl;
                const float TIME_STEP = 1.0f / 60.0f;
                const int32 VELOCITY_ITERATIONS = 8;
                const int32 POSITION_ITERATIONS = 3;

                world->Step(TIME_STEP, VELOCITY_ITERATIONS, POSITION_ITERATIONS);

            }

            // Render() Structure:
            // stable_sort(image_render_requests, CompareImageRequests)
            std::stable_sort(Renderer::imageRenderRequests.begin(), Renderer::imageRenderRequests.end(),
                             [](const Renderer::ImageRenderRequest &a, const Renderer::ImageRenderRequest &b)
                             {
                                 return a.sorting_order < b.sorting_order;
                             });

            // stable_sort(ui_render_requests, CompareUIRequests)
            std::stable_sort(Renderer::uiRenderRequests.begin(), Renderer::uiRenderRequests.end(),
                             [](const Renderer::UIRenderRequest &a, const Renderer::UIRenderRequest &b)
                             {
                                 return a.sorting_order < b.sorting_order;
                             });

            SDL_RenderSetScale(Renderer::renderer, Renderer::Camera::zoom_factor, Renderer::Camera::zoom_factor);

            // for request in image_render_requests:
            //     RenderImageRequest(request)
            for (const auto &request: Renderer::imageRenderRequests)
            {
                renderer.RenderImage(request);
            }
            Renderer::imageRenderRequests.clear();

            SDL_RenderSetScale(Renderer::renderer, 1, 1);
            // for request in ui_render_requests:
            //     RenderUIRequest(request)
            for (const auto &request: Renderer::uiRenderRequests)
            {
                renderer.RenderUIImage(request);
            }
            Renderer::uiRenderRequests.clear();

            // for request in text_render_requests:
            //     RenderTextRequest(request)
            while (!Renderer::textRenderRequests.empty())
            {
                auto &request = Renderer::textRenderRequests.front();
                renderer.RenderText(request);
                Renderer::textRenderRequests.pop(); // This modifies the queue!
            }

            // Render Pixel
            SDL_SetRenderDrawBlendMode(Renderer::renderer, SDL_BLENDMODE_BLEND);
            for (const auto &request: Renderer::pixelRenderRequests)
            {
                renderer.RenderPixel(request);
            }
            Renderer::pixelRenderRequests.clear();
            SDL_SetRenderDrawBlendMode(Renderer::renderer, SDL_BLENDMODE_NONE);


            // if (proceed_to_next_scene) LoadScene(next_scene_name)
            if (LuaManager::sceneChange)
            {
                currentScene.LoadNextScene(LuaManager::nextSceneName);
                currentScene.sceneChange = false;
                // continue;
            }


            break;
        }

        case ENDING:
        {
            //  break;
        }
    }

    // SDL_RenderPresent498(renderer)
    renderer.EndFrame();

    Input::LateUpdate();
}

int main(int argc, char *argv[])
{
    // event queue loop to drain the queue
    initialize();
    emscripten_set_main_loop(main_loop, 0, 1);

    // Free resources and close SDL
    renderer.Cleanup();
    SDL_Quit();
    TTF_Quit();

    return 0;
}
