// InputManager implementation
#include "input/InputManager.h"
#include "core/Logger.h"
#include <algorithm>

namespace Nova {

InputManager::InputManager() {
    keyCurrent_.fill(false);
    keyPrevious_.fill(false);
    mouseCurrent_.fill(false);
    mousePrevious_.fill(false);

    // Open any connected gamepads
    for (int i = 0; i < std::min(SDL_NumJoysticks(), MAX_GAMEPADS); i++) {
        if (SDL_IsGameController(i)) {
            gamepads_[i].controller = SDL_GameControllerOpen(i);
            if (gamepads_[i].controller) {
                NOVA_LOG("Gamepad connected: ", SDL_GameControllerName(gamepads_[i].controller));
            }
        }
    }

    // Default action mappings
    MapAction("move_left",  SDL_SCANCODE_LEFT);
    MapAction("move_right", SDL_SCANCODE_RIGHT);
    MapAction("move_up",    SDL_SCANCODE_UP);
    MapAction("move_down",  SDL_SCANCODE_DOWN);
    MapAction("jump",       SDL_SCANCODE_SPACE);
    MapAction("attack",     SDL_SCANCODE_Z);
    MapAction("interact",   SDL_SCANCODE_E);
    MapAction("pause",      SDL_SCANCODE_ESCAPE);
}

void InputManager::PreUpdate() {
    keyPrevious_ = keyCurrent_;
    mousePrevious_ = mouseCurrent_;
    mousePrevPos_ = mousePos_;
    scrollDelta_ = 0.0f;
    textInput_.clear();

    for (auto& gp : gamepads_) {
        gp.buttonsPrev = gp.buttons;
    }
}

void InputManager::ProcessEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.scancode < SDL_NUM_SCANCODES)
                keyCurrent_[event.key.keysym.scancode] = true;
            break;
        case SDL_KEYUP:
            if (event.key.keysym.scancode < SDL_NUM_SCANCODES)
                keyCurrent_[event.key.keysym.scancode] = false;
            break;
        case SDL_MOUSEMOTION:
            mousePos_ = Vec2((float)event.motion.x, (float)event.motion.y);
            mouseDelta_ = mousePos_ - mousePrevPos_;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button < 6)
                mouseCurrent_[event.button.button] = true;
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button < 6)
                mouseCurrent_[event.button.button] = false;
            break;
        case SDL_MOUSEWHEEL:
            scrollDelta_ = (float)event.wheel.y;
            break;
        case SDL_TEXTINPUT:
            textInput_ += event.text.text;
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            for (auto& gp : gamepads_) {
                if (gp.controller && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
                    gp.buttons[event.cbutton.button] = true;
            }
            break;
        case SDL_CONTROLLERBUTTONUP:
            for (auto& gp : gamepads_) {
                if (gp.controller && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX)
                    gp.buttons[event.cbutton.button] = false;
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            for (auto& gp : gamepads_) {
                if (gp.controller && event.caxis.axis < SDL_CONTROLLER_AXIS_MAX)
                    gp.axes[event.caxis.axis] = event.caxis.value / 32767.0f;
            }
            break;
        case SDL_CONTROLLERDEVICEADDED: {
            int idx = event.cdevice.which;
            if (idx < MAX_GAMEPADS && SDL_IsGameController(idx)) {
                gamepads_[idx].controller = SDL_GameControllerOpen(idx);
                NOVA_LOG("Gamepad added: ", idx);
            }
            break;
        }
        case SDL_CONTROLLERDEVICEREMOVED: {
            for (auto& gp : gamepads_) {
                if (gp.controller && SDL_GameControllerGetAttached(gp.controller) == SDL_FALSE) {
                    SDL_GameControllerClose(gp.controller);
                    gp.controller = nullptr;
                    NOVA_LOG("Gamepad removed");
                }
            }
            break;
        }
    }
}

// Keyboard
bool InputManager::IsKeyDown(SDL_Scancode key) const {
    return key < SDL_NUM_SCANCODES && keyCurrent_[key];
}
bool InputManager::IsKeyJustPressed(SDL_Scancode key) const {
    return key < SDL_NUM_SCANCODES && keyCurrent_[key] && !keyPrevious_[key];
}
bool InputManager::IsKeyJustReleased(SDL_Scancode key) const {
    return key < SDL_NUM_SCANCODES && !keyCurrent_[key] && keyPrevious_[key];
}

bool InputManager::IsKeyDown(const std::string& name) const {
    return IsKeyDown(KeyNameToScancode(name));
}
bool InputManager::IsKeyJustPressed(const std::string& name) const {
    return IsKeyJustPressed(KeyNameToScancode(name));
}
bool InputManager::IsKeyJustReleased(const std::string& name) const {
    return IsKeyJustReleased(KeyNameToScancode(name));
}

// Mouse
bool InputManager::IsMouseButtonDown(int btn) const {
    return btn < 6 && mouseCurrent_[btn];
}
bool InputManager::IsMouseButtonJustPressed(int btn) const {
    return btn < 6 && mouseCurrent_[btn] && !mousePrevious_[btn];
}
bool InputManager::IsMouseButtonJustReleased(int btn) const {
    return btn < 6 && !mouseCurrent_[btn] && mousePrevious_[btn];
}

// Gamepad
bool InputManager::IsGamepadConnected(int idx) const {
    return idx < MAX_GAMEPADS && gamepads_[idx].controller != nullptr;
}
bool InputManager::IsGamepadButtonDown(int btn, int idx) const {
    return idx < MAX_GAMEPADS && btn < SDL_CONTROLLER_BUTTON_MAX && gamepads_[idx].buttons[btn];
}
float InputManager::GetGamepadAxis(int axis, int idx) const {
    if (idx >= MAX_GAMEPADS || axis >= SDL_CONTROLLER_AXIS_MAX) return 0.0f;
    float val = gamepads_[idx].axes[axis];
    return Abs(val) < 0.15f ? 0.0f : val; // Deadzone
}
Vec2 InputManager::GetGamepadLeftStick(int idx) const {
    return {GetGamepadAxis(SDL_CONTROLLER_AXIS_LEFTX, idx),
            GetGamepadAxis(SDL_CONTROLLER_AXIS_LEFTY, idx)};
}
Vec2 InputManager::GetGamepadRightStick(int idx) const {
    return {GetGamepadAxis(SDL_CONTROLLER_AXIS_RIGHTX, idx),
            GetGamepadAxis(SDL_CONTROLLER_AXIS_RIGHTY, idx)};
}

// Action Mapping
void InputManager::MapAction(const std::string& action, SDL_Scancode key) {
    actionMap_[action] = key;
}
bool InputManager::IsActionDown(const std::string& action) const {
    auto it = actionMap_.find(action);
    return it != actionMap_.end() && IsKeyDown(it->second);
}
bool InputManager::IsActionJustPressed(const std::string& action) const {
    auto it = actionMap_.find(action);
    return it != actionMap_.end() && IsKeyJustPressed(it->second);
}

// Text Input
void InputManager::StartTextInput() { textInputActive_ = true; SDL_StartTextInput(); }
void InputManager::StopTextInput()  { textInputActive_ = false; SDL_StopTextInput(); }

// Key name to scancode conversion
SDL_Scancode InputManager::KeyNameToScancode(const std::string& name) const {
    SDL_Scancode code = SDL_GetScancodeFromName(name.c_str());
    if (code == SDL_SCANCODE_UNKNOWN) {
        // Try common aliases
        if (name == "left" || name == "arrow_left")   return SDL_SCANCODE_LEFT;
        if (name == "right" || name == "arrow_right")  return SDL_SCANCODE_RIGHT;
        if (name == "up" || name == "arrow_up")        return SDL_SCANCODE_UP;
        if (name == "down" || name == "arrow_down")    return SDL_SCANCODE_DOWN;
        if (name == "space")    return SDL_SCANCODE_SPACE;
        if (name == "enter")    return SDL_SCANCODE_RETURN;
        if (name == "escape")   return SDL_SCANCODE_ESCAPE;
        if (name == "tab")      return SDL_SCANCODE_TAB;
        if (name == "shift")    return SDL_SCANCODE_LSHIFT;
        if (name == "ctrl")     return SDL_SCANCODE_LCTRL;
        if (name == "alt")      return SDL_SCANCODE_LALT;
    }
    return code;
}

} // namespace Nova
