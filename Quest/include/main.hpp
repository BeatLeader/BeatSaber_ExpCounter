#pragma once

#include "_config.hpp"

#include "scotland2/shared/loader.hpp"

// beatsaber-hook is a modding framework that lets us call functions and fetch field values from in the game
// It also allows creating objects, configuration, and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Graphic.hpp"
#include "rapidjson-macros/shared/macros.hpp"

#include "paper2_scotland2/shared/logger.hpp"

namespace Qounters::Types {
    using PremadeFn = std::function<UnityEngine::UI::Graphic*(UnityEngine::GameObject*, UnparsedJSON)>;
    using PremadeUIFn = std::function<void(UnityEngine::GameObject*, UnparsedJSON)>;
    using PremadeUpdateFn = std::function<void(UnityEngine::UI::Graphic*, UnparsedJSON)>;
    using TemplateUIFn = std::function<void(UnityEngine::GameObject*)>;
}

inline modloader::ModInfo modInfo = {MOD_ID, VERSION, 0};

constexpr auto LLCLogger = Paper::ConstLoggerContext("ExpCounter");