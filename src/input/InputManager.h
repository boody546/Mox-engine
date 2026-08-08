#pragma once
#include "core/Math.h"
#include <SDL.h>
#include <unordered_map>
#include <string>
#include <array>

namespace Nova {

class InputManager {
public:
    InputManager();
    ~InputManager() = default;

    void PreUpdate();
    void ProcessEvent(const SDL_Event& event);

    // Keyboard
    bool IsKeyDown(SDL_Scancode key) const;
    bool IsKeyJustPressed(SDL_Scancode key) const;
    bool IsKeyJustReleased(SDL_Scancode key) const;
    bool IsKeyDown(const std::string& keyName) const;
    bool IsKeyJustPressed(const std::string& keyName) const;
    bool IsKeyJustReleased(const std::string& keyName) const;

    // Mouse
    Vec2  GetMousePosition() const { return mousePos_; }
    Vec2  GetMouseDelta() const { return mouseDelta_; }
    bool  IsMouseButtonDown(int button) const;
    bool  IsMouseButtonJustPressed(int button) const;
    bool  IsMouseButtonJustReleased(int button) const;
    float GetMouseScrollDelta() const { return scrollDelta_; }

    // Gamepad
    bool  IsGamepadConnected(int idx = 0) const;
    bool  IsGamepadButtonDown(int btn, int idx = 0) const;
    float GetGamepadAxis(int axis, int idx = 0) const;
    Vec2  GetGamepadLeftStick(int idx = 0) const;
    Vec2  GetGamepadRightStick(int idx = 0) const;

    // Action Mapping
    void MapAction(const std::string& action, SDL_Scancode key);
    bool IsActionDown(const std::string& action) const;
    bool IsActionJustPressed(const std::string& action) const;

    // Text Input
    void StartTextInput();
    void StopTextInput();
    const std::string& GetTextInput() const { return textInput_; }

private:
    SDL_Scancode KeyNameToScancode(const std::string& name) const;

    std::array<bool, SDL_NUM_SCANCODES> keyCurrent_{};
    std::array<bool, SDL_NUM_SCANCODES> keyPrevious_{};

    Vec2 mousePos_, mousePrevPos_, mouseDelta_;
    float scrollDelta_ = 0.0f;
    std::array<bool, 6> mouseCurrent_{};
    std::array<bool, 6> mousePrevious_{};

    static constexpr int MAX_GAMEPADS = 4;
    struct GamepadState {
        SDL_GameController* controller = nullptr;
        std::array<bool, SDL_CONTROLLER_BUTTON_MAX> buttons{};
        std::array<bool, SDL_CONTROLLER_BUTTON_MAX> buttonsPrev{};
        std::array<float, SDL_CONTROLLER_AXIS_MAX> axes{};
    };
    std::array<GamepadState, MAX_GAMEPADS> gamepads_{};

    std::unordered_map<std::string, SDL_Scancode> actionMap_;
    std::string textInput_;
    bool textInputActive_ = false;
};

} // namespace Nova
