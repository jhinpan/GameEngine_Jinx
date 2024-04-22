//
// Created by Jhin Pan on 2/14/24.
//

#ifndef MAIN_CPP_INPUT_H
#define MAIN_CPP_INPUT_H

#include "SDL2/SDL.h"
#include "glm/glm.hpp"
#include <vector>
#include <string>
#include <unordered_map>

// enum for possible states
enum INPUT_STATE {INPUT_STATE_UP, INPUT_STATE_JUST_BECAME_DOWN, INPUT_STATE_DOWN, INPUT_STATE_JUST_BECAME_UP};

class Input {
public:
    static inline const std::unordered_map<std::string, SDL_Scancode> __keycode_to_scancode = {
            // Directional (arrow) Keys
            {"up", SDL_SCANCODE_UP},
            {"down", SDL_SCANCODE_DOWN},
            {"right", SDL_SCANCODE_RIGHT},
            {"left", SDL_SCANCODE_LEFT},

            // Misc Keys
            {"escape", SDL_SCANCODE_ESCAPE},

            // Modifier Keys
            {"lshift", SDL_SCANCODE_LSHIFT},
            {"rshift", SDL_SCANCODE_RSHIFT},
            {"lctrl", SDL_SCANCODE_LCTRL},
            {"rctrl", SDL_SCANCODE_RCTRL},
            {"lalt", SDL_SCANCODE_LALT},
            {"ralt", SDL_SCANCODE_RALT},

            // Editing Keys
            {"tab", SDL_SCANCODE_TAB},
            {"return", SDL_SCANCODE_RETURN},
            {"enter", SDL_SCANCODE_RETURN},
            {"backspace", SDL_SCANCODE_BACKSPACE},
            {"delete", SDL_SCANCODE_DELETE},
            {"insert", SDL_SCANCODE_INSERT},

            // Character Keys
            {"space", SDL_SCANCODE_SPACE},
            {"a", SDL_SCANCODE_A},
            {"b", SDL_SCANCODE_B},
            {"c", SDL_SCANCODE_C},
            {"d", SDL_SCANCODE_D},
            {"e", SDL_SCANCODE_E},
            {"f", SDL_SCANCODE_F},
            {"g", SDL_SCANCODE_G},
            {"h", SDL_SCANCODE_H},
            {"i", SDL_SCANCODE_I},
            {"j", SDL_SCANCODE_J},
            {"k", SDL_SCANCODE_K},
            {"l", SDL_SCANCODE_L},
            {"m", SDL_SCANCODE_M},
            {"n", SDL_SCANCODE_N},
            {"o", SDL_SCANCODE_O},
            {"p", SDL_SCANCODE_P},
            {"q", SDL_SCANCODE_Q},
            {"r", SDL_SCANCODE_R},
            {"s", SDL_SCANCODE_S},
            {"t", SDL_SCANCODE_T},
            {"u", SDL_SCANCODE_U},
            {"v", SDL_SCANCODE_V},
            {"w", SDL_SCANCODE_W},
            {"x", SDL_SCANCODE_X},
            {"y", SDL_SCANCODE_Y},
            {"z", SDL_SCANCODE_Z},
            {"0", SDL_SCANCODE_0},
            {"1", SDL_SCANCODE_1},
            {"2", SDL_SCANCODE_2},
            {"3", SDL_SCANCODE_3},
            {"4", SDL_SCANCODE_4},
            {"5", SDL_SCANCODE_5},
            {"6", SDL_SCANCODE_6},
            {"7", SDL_SCANCODE_7},
            {"8", SDL_SCANCODE_8},
            {"9", SDL_SCANCODE_9},
            {"/", SDL_SCANCODE_SLASH},
            {";", SDL_SCANCODE_SEMICOLON},
            {"=", SDL_SCANCODE_EQUALS},
            {"-", SDL_SCANCODE_MINUS},
            {".", SDL_SCANCODE_PERIOD},
            {",", SDL_SCANCODE_COMMA},
            {"[", SDL_SCANCODE_LEFTBRACKET},
            {"]", SDL_SCANCODE_RIGHTBRACKET},
            {"\\", SDL_SCANCODE_BACKSLASH},
            {"'", SDL_SCANCODE_APOSTROPHE}
    };

    static void Init(); // Call before main loop begins.
    static void ProcessEvent(const SDL_Event &event); // Call every frame at start of event loop.
    static void LateUpdate(); // Call at very end of frame.

    /* Interface */
    static bool GetKey(SDL_Scancode keycode);
    static bool GetKeyDown(SDL_Scancode keycode);
    static bool GetKeyUp(SDL_Scancode keycode);

    static glm::vec2 GetMousePosition();

    static bool GetMouseButton(int button);
    static bool GetMouseButtonDown(int button);
    static bool GetMouseButtonUp(int button);
    static float GetMouseScrollDelta();

private:
    static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states; // Container of key -> state pairs
    static inline std::vector<SDL_Scancode> just_became_down_scancodes;
    static inline std::vector<SDL_Scancode> just_became_up_scancodes;

    static inline glm::vec2 mouse_position;
    static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
    static inline std::vector<int> just_became_down_buttons;
    static inline std::vector<int> just_became_up_buttons;

    static inline float mouse_scroll_this_frame = 0.0f;

};

void Input::Init(){
    /* All inputs begin in the UP state. */
    for (int code = SDL_SCANCODE_UNKNOWN; code < SDL_NUM_SCANCODES; code++) {
        keyboard_states[(SDL_Scancode)code] = INPUT_STATE_UP;
    }

    // Initialize mouse position to (0,0)
    mouse_position = glm::vec2(0, 0);

    // Initialize mouse button states to UP
    mouse_button_states[SDL_BUTTON_LEFT] = INPUT_STATE_UP;
    mouse_button_states[SDL_BUTTON_MIDDLE] = INPUT_STATE_UP;
    mouse_button_states[SDL_BUTTON_RIGHT] = INPUT_STATE_UP;

    // Initialize mouse scroll delta to 0
    mouse_scroll_this_frame = 0.0f;

}

void Input::ProcessEvent(const SDL_Event& event){
    switch (event.type) {
        case SDL_KEYDOWN:
            if (!event.key.repeat) {
                keyboard_states[event.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_DOWN;
                just_became_down_scancodes.push_back(event.key.keysym.scancode);
            }
            break;
        case SDL_KEYUP:
            keyboard_states[event.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_scancodes.push_back(event.key.keysym.scancode);
            break;
        case SDL_MOUSEMOTION:
            mouse_position.x = event.motion.x;
            mouse_position.y = event.motion.y;
            break;
        case SDL_MOUSEBUTTONDOWN:
            mouse_button_states[event.button.button] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_buttons.push_back(event.button.button);
            break;
        case SDL_MOUSEBUTTONUP:
            mouse_button_states[event.button.button] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_buttons.push_back(event.button.button);
            break;
        case SDL_MOUSEWHEEL:
            mouse_scroll_this_frame += event.wheel.preciseY;
            break;
    }
}


// As next frame arrives, update "just" keys to their natural state.
void Input::LateUpdate()
{
    // Keyboard
    for (const SDL_Scancode& code : just_became_down_scancodes){
        keyboard_states[code] = INPUT_STATE_DOWN;
    }
    just_became_down_scancodes.clear();

    for (const SDL_Scancode& code : just_became_up_scancodes){
        keyboard_states[code] = INPUT_STATE_UP;
    }
    just_became_up_scancodes.clear();

    // Mouse Buttons
    for (const int& button : just_became_down_buttons) {
        mouse_button_states[button] = INPUT_STATE_DOWN;
    }
    just_became_down_buttons.clear();

    for (const int& button : just_became_up_buttons) {
        mouse_button_states[button] = INPUT_STATE_UP;
    }
    just_became_up_buttons.clear();

    // Mouse Scroll
    mouse_scroll_this_frame = 0;
}



// The following methods are used to query the state of a key.
// They are static, so they can be called from anywhere in the code.
// Generated by Github Copilot
bool Input::GetKey(SDL_Scancode keycode){
    return keyboard_states[keycode] == INPUT_STATE_DOWN || keyboard_states[keycode] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetKeyDown(SDL_Scancode keycode){
    return keyboard_states[keycode] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetKeyUp(SDL_Scancode keycode){
    return keyboard_states[keycode] == INPUT_STATE_JUST_BECAME_UP;
}


glm::vec2 Input::GetMousePosition() {
    return mouse_position;
}

bool Input::GetMouseButton(int button) {
    return mouse_button_states[button] == INPUT_STATE_DOWN || mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonDown(int button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonUp(int button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP;
}

float Input::GetMouseScrollDelta() {
    return mouse_scroll_this_frame;
}


#endif //MAIN_CPP_INPUT_H
