#include "main.hpp"
#include "ModConfig.hpp"
#include "config-utils/shared/config-utils.hpp"

#include "GlobalNamespace/PracticeViewController.hpp"
#include "HMUI/RangeValuesTextSlider.hpp"

#include "metacore/shared/events.hpp"
#include "metacore/shared/internals.hpp"
#include "metacore/shared/strings.hpp"
#include "metacore/shared/ui.hpp"

#include "qounters++/shared/options.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "rapidjson-macros/shared/macros.hpp"

#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML.hpp"

#include <algorithm>
#include <string>
#include <vector>
#include <dlfcn.h>

using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace HMUI;

// ==================== Left Leader Counter (Qounters++ Premade) ====================

DECLARE_JSON_STRUCT(ExpOpts) {
    VALUE_DEFAULT(int, Decimals, 2);
};

static std::vector<std::pair<TMPro::TextMeshProUGUI*, int>> expInstances;

static constexpr int kExpPerSecond = 9;
static constexpr double kAccMult[][2] = {
    {1.00, 1.00},  {0.95, 1.00},  {0.935, 0.983}, {0.92, 0.948},
    {0.90, 0.89},  {0.85, 0.80},  {0.80, 0.73},   {0.75, 0.69},
    {0.70, 0.67},  {0.65, 0.65},  {0.60, 0.60},   {0.55, 0.515},
    {0.50, 0.44},  {0.45, 0.38},  {0.40, 0.34},   {0.35, 0.315},
    {0.30, 0.30},  {0.00, 0.30},
};
static constexpr int kAccMultSize = sizeof(kAccMult) / sizeof(kAccMult[0]);

static float expLastTime = 0.0f;
static float expDuration = 0.0f;

static float CalculateMultFromAccuracy(float value) {
    if (kAccMult[0][0] <= value) return static_cast<float>(kAccMult[0][1]);
    if (kAccMult[kAccMultSize - 1][0] >= value) return static_cast<float>(kAccMult[kAccMultSize - 1][1]);

    int i = 0;
    for (; i < kAccMultSize; i++) {
        if (kAccMult[i][0] <= value) break;
    }
    if (i == kAccMultSize) return 0.0f;
    if (i == 0) i = 1;

    double t = (value - kAccMult[i - 1][0]) / (kAccMult[i][0] - kAccMult[i - 1][0]);
    return static_cast<float>(kAccMult[i - 1][1] + t * (kAccMult[i][1] - kAccMult[i - 1][1]));
}

static float CalculateDuration(float prev, float next, float timeScale) {
    if (timeScale <= 0.0f) timeScale = 1.0f;
    return (next - prev <= 1.0f) ? (next - prev) / timeScale : 1.0f / timeScale;
}

static float CalculateExp(float duration, float accuracy) {
    return kExpPerSecond * duration * CalculateMultFromAccuracy(accuracy);
}

static void RefreshExpText(TMPro::TextMeshProUGUI* text, int decimals) {
    int totalScore = MetaCore::Internals::leftScore + MetaCore::Internals::rightScore;
    int totalMaxScore = MetaCore::Internals::leftMaxScore + MetaCore::Internals::rightMaxScore;
    float accuracy = totalMaxScore > 0 ? static_cast<float>(totalScore) / totalMaxScore : 1.0f;
    float exp = CalculateExp(expDuration, accuracy);
    text->text = MetaCore::Strings::FormatDecimals(exp, decimals) + "xp";
}

static UI::Graphic* CreateExp(GameObject* parent, UnparsedJSON options) {
    auto opts = options.Parse<ExpOpts>();
    auto text = BSML::Lite::CreateText(parent->transform, "");
    text->enableWordWrapping = false;
    text->fontSize = 15;
    text->alignment = TMPro::TextAlignmentOptions::Center;

    RefreshExpText(text, opts.Decimals);
    expInstances.emplace_back(text, opts.Decimals);
    return text;
}

static void UpdateExp(UI::Graphic* graphic, UnparsedJSON options) {
    auto text = reinterpret_cast<TMPro::TextMeshProUGUI*>(graphic);
    auto opts = options.Parse<ExpOpts>();
    for (auto& [t, d] : expInstances) {
        if (t == text) {
            d = opts.Decimals;
            break;
        }
    }
    RefreshExpText(text, opts.Decimals);
}

// ==================== LeftLeader Template (Qounters++ "Add Counter" UI) ====================

static void (*API_AddGroup)(Qounters::Options::Group) = nullptr;
static void (*API_CloseTemplateModal)() = nullptr;

static void ExpTemplateUI(GameObject* parent) {
    static int anchor = (int)Qounters::Options::Group::Anchors::Left;

    BSML::Lite::AddHoverHint(
        MetaCore::UI::CreateDropdownEnum(parent, "Starting Anchor", anchor,
            Qounters::Options::AnchorStrings, [](int val) { anchor = val; }),
        "Select the anchor to create this counter group on"
    );

    auto buttons = BSML::Lite::CreateHorizontalLayoutGroup(parent);
    buttons->spacing = 3;

    BSML::Lite::CreateUIButton(buttons, "Cancel", []() {
        API_CloseTemplateModal();
    });

    BSML::Lite::CreateUIButton(buttons, "Create", "ActionButton", []() {
        Qounters::Options::Group group;
        group.Anchor = anchor;
        group.Position = ConfigUtils::Vector2(0, 0);

        auto& comp = group.Components.emplace_back();
        comp.Type = (int)Qounters::Options::Component::Types::Premade;
        Qounters::Options::Premade premadeOpts;
        premadeOpts.SourceMod = MOD_ID;
        premadeOpts.Name = "Exp counter";
        premadeOpts.Options = ExpOpts{};
        comp.Options = premadeOpts;

        API_AddGroup(group);
        API_CloseTemplateModal();
    });
}

// ==================== Qounters++ Init ====================

static void InitQounters() {
    using RegisterPremadeFn = void(*)(
        std::string, std::string,
        Qounters::Types::PremadeFn,
        Qounters::Types::PremadeUIFn,
        Qounters::Types::PremadeUpdateFn
    );
    using RegisterTemplateFn = void(*)(
        std::string, std::string,
        Qounters::Types::TemplateUIFn
    );

    for (auto& mod : modloader::get_loaded()) {
        if (mod.info.id != "Qounters++" || !mod.info.version.starts_with("1."))
            continue;

        auto RegisterPremade = reinterpret_cast<RegisterPremadeFn>(dlsym(mod.handle,
            "_ZN8Qounters3API15RegisterPremadeE"
            "NSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"
            "S7_"
            "NS1_8functionIFPN11UnityEngine2UI7GraphicEPNS9_10GameObjectE12UnparsedJSONEEE"
            "NS8_IFvSE_SF_EEE"
            "NS8_IFvSC_SF_EEE"));

        auto RegisterTemplate = reinterpret_cast<RegisterTemplateFn>(dlsym(mod.handle,
            "_ZN8Qounters3API16RegisterTemplateE"
            "NSt6__ndk112basic_stringIcNS1_11char_traitsIcEENS1_9allocatorIcEEEE"
            "S7_"
            "NS1_8functionIFvPN11UnityEngine10GameObjectEEEE"));

        API_AddGroup = reinterpret_cast<decltype(API_AddGroup)>(dlsym(mod.handle,
            "_ZN8Qounters3API8AddGroupENS_7Options5GroupE"));

        API_CloseTemplateModal = reinterpret_cast<decltype(API_CloseTemplateModal)>(dlsym(mod.handle,
            "_ZN8Qounters3API18CloseTemplateModalEv"));

        if (!RegisterPremade) {
            LLCLogger.error("Qounters++ {} found but failed to resolve RegisterPremade.", mod.info.version);
            return;
        }

        LLCLogger.info("Qounters++ {} detected, registering Exp counter.", mod.info.version);

        RegisterPremade(MOD_ID, "Exp counter", CreateExp, nullptr, UpdateExp);

        if (RegisterTemplate && API_AddGroup && API_CloseTemplateModal) {
            RegisterTemplate(MOD_ID, "Exp counter", ExpTemplateUI);
        } else {
            LLCLogger.warn("Could not resolve template API, template not registered.");
        }

        MetaCore::Events::AddCallback(MetaCore::Events::ScoreChanged, []() {
            float currentTime = MetaCore::Internals::songTime;
            float timeScale = MetaCore::Internals::songSpeed;

            if (expLastTime == 0.0f) {
                expLastTime = currentTime;
            } else if (currentTime > expLastTime) {
                expDuration += CalculateDuration(expLastTime, currentTime, timeScale);
                expLastTime = currentTime;
            }

            for (auto& [text, decimals] : expInstances)
                RefreshExpText(text, decimals);
        });

        MetaCore::Events::AddCallback(MetaCore::Events::GameplaySceneEnded, []() {
            expInstances.clear();
            expLastTime = 0.0f;
            expDuration = 0.0f;
        });

        return;
    }
    LLCLogger.info("Qounters++ not detected.");
}

// ==================== Practice Mode Tweaks ====================

static void AddIncDecButtonsToSlider(RangeValuesTextSlider *slider)
{
    if (!slider)
        return;

    auto parent = slider->get_transform();
    if (!parent)
        return;

    auto createStepAdjust = [slider](bool increment)
    {
        float step = slider->numberOfSteps > 0 ? 1.0f / static_cast<float>(slider->numberOfSteps)
                                               : 1.0f / static_cast<float>(slider->maxValue);
        float current = slider->get_normalizedValue();
        float next = increment ? current + step : current - step;
        next = std::clamp(next, 0.0f, 1.0f);
        slider->SetNormalizedValue(next);
    };

    auto decBtn = BSML::Lite::CreateUIButton(parent, "◀", {-6, -5}, {10, 10}, [createStepAdjust]()
                                             { createStepAdjust(false); });
    auto incBtn = BSML::Lite::CreateUIButton(parent, "▶", {116, -5}, {10, 10}, [createStepAdjust]()
                                             { createStepAdjust(true); });

    if (decBtn && incBtn)
    {
        decBtn->get_transform()->SetAsFirstSibling();
        incBtn->get_transform()->SetAsLastSibling();
    }
}

MAKE_HOOK_MATCH(PracticeViewControllerDidActivate, &PracticeViewController::DidActivate, void, PracticeViewController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
{
    PracticeViewControllerDidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    self->_speedSlider->numberOfSteps = 599;
    self->_speedSlider->minValue = 0.01f;
    self->_speedSlider->maxValue = 3.0f;

    if (firstActivation)
    {
        AddIncDecButtonsToSlider(self->_speedSlider);
        AddIncDecButtonsToSlider(self->_songStartSlider);
    }
}

// ==================== Mod Lifecycle ====================

MOD_EXPORT void setup(CModInfo *info) noexcept
{
    *info = modInfo.to_c();
    getModConfig().Init(modInfo);
    Paper::Logger::RegisterFileContextId(LLCLogger.tag);
    LLCLogger.info("Completed setup!");
}

MOD_EXPORT "C" void late_load()
{
    il2cpp_functions::Init();
    BSML::Init();

    InitQounters();

    // INSTALL_HOOK(LLCLogger, PracticeViewControllerDidActivate);
}
