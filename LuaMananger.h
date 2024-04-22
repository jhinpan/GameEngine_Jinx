//
// Created by Jhin Pan on 3/6/24.
//

#ifndef MAIN_CPP_LUAMANANGER_H
#define MAIN_CPP_LUAMANANGER_H

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include <vector>

class LuaManager
{
public:
    static inline lua_State *lua_state;
    static inline std::string nextSceneName;
    static inline std::string currentSceneName;
    static inline bool sceneChange = false;


    LuaManager()
    {

    }

    ~LuaManager()
    {

    }

    static lua_State *GetLuaState()
    {
        return lua_state;
    }

    static std::string GetCurrentSceneName()
    {
        return currentSceneName;
    }

    static void LoadScene(const std::string &sceneName)
    {
        nextSceneName = sceneName;
        sceneChange = true;
    }
};


#endif //MAIN_CPP_LUAMANANGER_H
