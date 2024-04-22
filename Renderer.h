#ifndef MAIN_CPP_RENDERER_H
#define MAIN_CPP_RENDERER_H

#include <unordered_map>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <queue>
#include <cmath>
#include "SDL2/SDL.h"
#include "SDL2_image/SDL_image.h"
#include "SDL2_ttf/SDL_ttf.h"
#include "EngineUtils.h"
#include "Helper.h"
#include "Actor.h"

const SDL_Color DEFAULT_COLOR = {255, 255, 255, 255}; // White

class Renderer
{
public:
    // Clear color components
    Uint8 clear_color_r = 255;
    Uint8 clear_color_g = 255;
    Uint8 clear_color_b = 255;

    SDL_Window *window = nullptr;
    int window_width = 0;
    int window_height = 0;
    static inline SDL_Renderer *renderer = nullptr;
    std::unordered_map<std::string, SDL_Texture *> textures; // Manage images
    // std::unordered_map<std::string, TTF_Font *> fonts; // Manage fonts
    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fonts; // <fontName, <fontSize, TTF*>>

    std::vector<std::string> introTexts; // Store intro texts
    int currentIntroIndex = 0; // Track the current intro index

    class TextRenderRequest{
    public:
        std::string text;
        std::string font;
        SDL_Color color;
        int size;
        int x;
        int y;

        TextRenderRequest();
        TextRenderRequest(std::string text, int x, int y, std::string font, int size, SDL_Color color){
            this->text = std::move(text);
            this->x = x;
            this->y = y;
            this->font = std::move(font);
            this->size = size;
            this->color = color;
        };
    };

    static inline std::queue<Renderer::TextRenderRequest> textRenderRequests;

    static void ReadTextRenderRequest(std::string text, int x, int y, std::string font, int size, int r, int g, int b, int a){
        SDL_Color color = {static_cast<Uint8>(std::round(r)), static_cast<Uint8>(std::round(g)), static_cast<Uint8>(std::round(b)), static_cast<Uint8>(std::round(a))};
        Renderer::TextRenderRequest textRenderRequest = Renderer::TextRenderRequest(std::move(text), x, y, std::move(font), size, color);
        textRenderRequests.push(textRenderRequest);
    };

    class UIRenderRequest{
    public:
        std::string image; // image name for UI
        float x;
        float y;
        SDL_Color color = DEFAULT_COLOR;
        int sorting_order = 0;

        UIRenderRequest();
        // Constructor for UIRenderEx
        UIRenderRequest(std::string image, float x, float y, SDL_Color color, int sorting_order){
            this->image = std::move(image);
            this->x = x;
            this->y = y;
            this->color = color;
            this->sorting_order = sorting_order;
        };
        // Constructor for UIRender
        UIRenderRequest(std::string image, float x, float y){
            this->image = std::move(image);
            this->x = x;
            this->y = y;
        };
    };

    static inline std::vector<Renderer::UIRenderRequest> uiRenderRequests;

    static void ReadUIRenderRequest(std::string image, float x, float y){
        Renderer::UIRenderRequest uiRenderRequest = Renderer::UIRenderRequest(std::move(image), x, y);
        uiRenderRequests.push_back(uiRenderRequest);
    };

    static void ReadUIRenderRequestEx(std::string image, float x, float y, float r, float g, float b, float a, int sorting_order){
        SDL_Color color = {static_cast<Uint8>(r), static_cast<Uint8>(g),
                           static_cast<Uint8>(b), static_cast<Uint8>(a)};
        Renderer::UIRenderRequest uiRenderRequest = Renderer::UIRenderRequest(std::move(image), x, y, color, sorting_order);
        uiRenderRequests.emplace_back(uiRenderRequest);
    };

    void RenderUIImage(Renderer::UIRenderRequest uiRenderRequest);


    class ImageRenderRequest{
    public:
        std::string image;
        SDL_Color color = DEFAULT_COLOR;
        float x;
        float y;
        float sorting_order;
        int rotation = 0;
        float scale_x = 1.0f;
        float scale_y = 1.0f;
        float pivot_x = 0.5f;
        float pivot_y = 0.5f;

        ImageRenderRequest();
        ImageRenderRequest(std::string image, float x, float y){
            this->image = std::move(image);
            this->x = x;
            this->y = y;
        };
        ImageRenderRequest(std::string image, SDL_Color color, float x, float y, float sorting_order,
                           float rotation, float scale_x, float scale_y, float pivot_x, float pivot_y){
            this->image = std::move(image);
            this->color = color;
            this->x = x;
            this->y = y;
            this->sorting_order = sorting_order;
            this->rotation = static_cast<int>(rotation);
            this->scale_x = scale_x;
            this->scale_y = scale_y;
            this->pivot_x = pivot_x;
            this->pivot_y = pivot_y;
        };
    };

    static inline std::vector<Renderer::ImageRenderRequest> imageRenderRequests;

    static void ReadImageRenderRequest(std::string image, float x, float y){
        Renderer::ImageRenderRequest imageRenderRequest = Renderer::ImageRenderRequest(std::move(image), x, y);
        imageRenderRequests.push_back(imageRenderRequest);
    };

    static void ReadImageRenderRequestEx(std::string image, float x, float y, float rotation, float scale_x, float scale_y, float pivot_x, float pivot_y,
                                         float r, float g, float b, float a, float sorting_order){
        SDL_Color color = {static_cast<Uint8>(r), static_cast<Uint8>(g),
                           static_cast<Uint8>(b), static_cast<Uint8>(a)};
        Renderer::ImageRenderRequest imageRenderRequest = Renderer::ImageRenderRequest(std::move(image), color, x, y, sorting_order,
                                                                                       rotation, scale_x, scale_y, pivot_x, pivot_y);
        imageRenderRequests.emplace_back(imageRenderRequest);
    };

    void RenderImage(const Renderer::ImageRenderRequest& imageRenderRequest);

    class PixelRenderRequest{
    public:
        float x;
        float y;
        SDL_Color color;

        PixelRenderRequest();
        PixelRenderRequest(float x, float y, SDL_Color color){
            this->x = x;
            this->y = y;
            this->color = color;
        };
    };

    static inline std::vector<Renderer::PixelRenderRequest> pixelRenderRequests;

    static void ReadPixelRenderRequest(float x, float y, float r, float g, float b, float a){
        SDL_Color color = {static_cast<Uint8>(r), static_cast<Uint8>(g),
                           static_cast<Uint8>(b), static_cast<Uint8>(a)};
        Renderer::PixelRenderRequest pixelRenderRequest = Renderer::PixelRenderRequest(x, y, color);
        pixelRenderRequests.push_back(pixelRenderRequest);
    };

    void RenderPixel(const Renderer::PixelRenderRequest& pixelRenderRequest);

    class Camera{
    public:
        static inline float cam_pos_x = 0.0f;
        static inline float cam_pos_y = 0.0f;
        static inline float zoom_factor = 1.0f;

        static void SetCameraPosition(float x, float y){
            cam_pos_x = x;
            cam_pos_y = y;
            // std::cout << "Camera position set to: " << x << ", " << y << std::endl;
        }

        static float GetPositionX(){
            // std::cout << "Current Camera position x: " << cam_pos_x << std::endl;
            return cam_pos_x;
        }

        static float GetPositionY(){
            // std::cout << "Current Camera position y: " << cam_pos_y << std::endl;
            return cam_pos_y;
        }

        static void SetZoomFactor(float zoomFactor){
            zoom_factor = zoomFactor;
        }

        static float GetZoomFactor(){
            return zoom_factor;
        }
    };


    void Initialize(const std::string &title, int width, int height);

    void LoadClearColor();

    void StartFrame();

    void EndFrame();

    void Cleanup();

    void RenderText(Renderer::TextRenderRequest textRenderRequest);

    void CleanupIntroTexts();

};


void Renderer::Initialize(const std::string &title, int width, int height)
{
    window = Helper::SDL_CreateWindow498(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                              SDL_WINDOW_SHOWN);
    window_width = width;
    window_height = height;
    if (!window)
    {
        // Handle window creation error
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    renderer = Helper::SDL_CreateRenderer498(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        // Handle renderer creation error
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        window = nullptr;
        return;
    }

    // After initializing the renderer, load clear color
    LoadClearColor();
}

void Renderer::LoadClearColor()
{
    // Check if rendering config exists
    if (std::filesystem::exists("resources/rendering.config"))
    {
        rapidjson::Document renderingConfig;
        EngineUtils::ReadJsonFile("resources/rendering.config", renderingConfig);

        // Load clear color components if they exist
        if (renderingConfig.HasMember("clear_color_r")) clear_color_r = renderingConfig["clear_color_r"].GetUint();
        if (renderingConfig.HasMember("clear_color_g")) clear_color_g = renderingConfig["clear_color_g"].GetUint();
        if (renderingConfig.HasMember("clear_color_b")) clear_color_b = renderingConfig["clear_color_b"].GetUint();
    }
}

void Renderer::StartFrame()
{
    // Clear the renderer with a default color, for example, white (255, 255, 255)
    SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255); // Alpha is set to 255
    SDL_RenderClear(renderer);
}

void Renderer::EndFrame()
{
    // Present the renderer contents to the window
    Helper::SDL_RenderPresent498(renderer);
    // SDL_Delay(1); // chill-ax for 1 millisecond
}

void Renderer::Cleanup()
{
    for (auto &texture: textures)
    {
        SDL_DestroyTexture(texture.second);
    }
    textures.clear();
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
}

void Renderer::RenderText(Renderer::TextRenderRequest textRenderRequest)
{
    std::string fontName = textRenderRequest.font;
    std::string fontPath = "resources/fonts/" + fontName + ".ttf";
    int fontSize = textRenderRequest.size;

    // Check if the font is already loaded
    if (fonts.find(fontName) == fonts.end())
    {
        TTF_Font *font = TTF_OpenFont(fontPath.c_str(), fontSize); // load font from disk
        fonts[fontName][fontSize] = font; // load font now
    }

    // Check if the font and size are loaded
    if (fonts.find(fontName) == fonts.end() || fonts[fontName].find(fontSize) == fonts[fontName].end())
    {
        std::cout << "error: font " << fontName << " at size " << fontSize << " not loaded" << std::endl;
        exit(1);
    }

    TTF_Font *font = fonts[fontName][fontSize];
    SDL_Surface *surface = TTF_RenderText_Solid(font, textRenderRequest.text.c_str(),textRenderRequest.color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {textRenderRequest.x, textRenderRequest.y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void Renderer::CleanupIntroTexts()
{
    for (auto &font: fonts)
    {
        TTF_Font *fontPtr = font.second.begin()->second;
        if (fontPtr != nullptr)
        {
            TTF_CloseFont(fontPtr);
        }
    }
    fonts.clear();
}

// Image.DrawUI(image_name, x, y)
// All numeric parameters are floats, but become downcast to ints immediately in C+
// Draws an image to UI via screen coordinates (not affected by camera)
void Renderer::RenderUIImage(Renderer::UIRenderRequest uiRenderRequest)
{
    std::string image = uiRenderRequest.image;
    int x = (int)uiRenderRequest.x; // explicit downcast to int
    int y = (int)uiRenderRequest.y; // explicit downcast to int

    std::string fullPath = "resources/images/" + image + ".png";

    SDL_Texture *textureUI = IMG_LoadTexture(renderer, fullPath.c_str());

    textures[image] = textureUI; // Store the loaded texture in the textures map

    int textureWidth, textureHeight;
    SDL_QueryTexture(textureUI, nullptr, nullptr, &textureWidth, &textureHeight);
    SDL_Rect dstRect = {x, y, textureWidth, textureHeight};

    // if uiRenderRequest.color is not the default color, then apply the color
    if (uiRenderRequest.color.r != DEFAULT_COLOR.r || uiRenderRequest.color.g != DEFAULT_COLOR.g ||
    uiRenderRequest.color.b != DEFAULT_COLOR.b)
    {
        SDL_SetTextureColorMod(textureUI, uiRenderRequest.color.r, uiRenderRequest.color.g, uiRenderRequest.color.b);
    }

    if (uiRenderRequest.color.a != DEFAULT_COLOR.a)
    {
        SDL_SetTextureAlphaMod(textureUI, uiRenderRequest.color.a);
    }

    SDL_RenderCopy(renderer, textureUI, nullptr, &dstRect);

    // Reset color and alpha modifications
    SDL_SetTextureColorMod(textureUI, DEFAULT_COLOR.r, DEFAULT_COLOR.g, DEFAULT_COLOR.b);
    SDL_SetTextureAlphaMod(textureUI, DEFAULT_COLOR.a);
}

// Image.Draw&DrawEx(image_name, x, y)
void Renderer::RenderImage(const Renderer::ImageRenderRequest& imageRenderRequest)
{
    std::string image = imageRenderRequest.image;
    float x = imageRenderRequest.x; // pos_x
    float y = imageRenderRequest.y; // pos_y
    float pivot_x = imageRenderRequest.pivot_x;
    float pivot_y = imageRenderRequest.pivot_y;
    float scale_x = imageRenderRequest.scale_x;
    float scale_y = imageRenderRequest.scale_y;

    if (textures.find(image) == textures.end())
    {
        std::string fullPath = "resources/images/" + image + ".png";
        SDL_Texture *texture = IMG_LoadTexture(renderer, fullPath.c_str());

        textures[image] = texture; // Store the loaded texture in the textures map
    }

    SDL_Texture* texture = textures[image];

    int textureWidth, textureHeight;
    SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);
    SDL_Rect dstRect;

    int flip_mode = SDL_FLIP_NONE;
    if (scale_x < 0)
    {
        scale_x = -scale_x;
        flip_mode = SDL_FLIP_HORIZONTAL;
    }
    if (scale_y < 0)
    {
        scale_y = -scale_y;
        flip_mode = SDL_FLIP_VERTICAL;
    }

    int final_pixel_pivot_x = static_cast<int>(pivot_x * (float)textureWidth * scale_x);
    int final_pixel_pivot_y = static_cast<int>(pivot_y * (float)textureHeight * scale_y);

    dstRect.x = static_cast<int>((x - Camera::cam_pos_x) * 100.0f + (float)window_width * 0.5f * (1.0f / Camera::zoom_factor)
            - (float)final_pixel_pivot_x);
    dstRect.y = static_cast<int>((y - Camera::cam_pos_y) * 100.0f + (float)window_height * 0.5f * (1.0f / Camera::zoom_factor)
            - (float)final_pixel_pivot_y);

    dstRect.w = static_cast<int>((float)textureWidth * scale_x);
    dstRect.h = static_cast<int>((float)textureHeight * scale_y);

    SDL_Point center = {final_pixel_pivot_x, final_pixel_pivot_y};

    // if imageRenderRequest.color is not the default color, then apply the color
    if (imageRenderRequest.color.r != DEFAULT_COLOR.r || imageRenderRequest.color.g != DEFAULT_COLOR.g ||
        imageRenderRequest.color.b != DEFAULT_COLOR.b)
    {
        SDL_SetTextureColorMod(texture, imageRenderRequest.color.r, imageRenderRequest.color.g, imageRenderRequest.color.b);
    }

    if (imageRenderRequest.color.a != DEFAULT_COLOR.a)
    {
        SDL_SetTextureAlphaMod(texture, imageRenderRequest.color.a);
    }

    Helper::SDL_RenderCopyEx498(0, "actor", renderer, texture, nullptr, &dstRect,
                                imageRenderRequest.rotation, &center, static_cast<SDL_RendererFlip> (flip_mode));

    // Reset color and alpha modifications
    SDL_SetTextureColorMod(texture, DEFAULT_COLOR.r, DEFAULT_COLOR.g, DEFAULT_COLOR.b);
    SDL_SetTextureAlphaMod(texture, DEFAULT_COLOR.a);
}

void Renderer::RenderPixel(const Renderer::PixelRenderRequest& pixelRenderRequest)
{
    float x = pixelRenderRequest.x;
    float y = pixelRenderRequest.y;
    SDL_Color color = pixelRenderRequest.color;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawPoint(renderer, static_cast<int>(x), static_cast<int>(y));
    SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255); // Reset the color
}


#endif //MAIN_CPP_RENDERER_H
