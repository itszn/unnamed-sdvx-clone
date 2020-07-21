#pragma once

// Types of input device
DefineEnum(InputDevice,
	Keyboard,
	Mouse,
	Controller)

typedef Ref<int32> MouseLockHandle;

/*
	Class that handles game keyboard (and soon controller input)
*/
class Input : Unique
{
public:
	DefineEnum(Button,
		BT_0,
		BT_1,
		BT_2,
		BT_3,
		FX_0,
		FX_1,
        BT_S, //Start Button
		LS_0Neg, // Left laser- 
		LS_0Pos, // Left laser+		(|---->)
		LS_1Neg, // Right laser-	(<----|)
		LS_1Pos, // Right laser+
		Back,
		Length)

	~Input();
	void Init(Graphics::Window& wnd);
	void Cleanup();

	// Poll/Update input
	void Update(float deltaTime);

	bool GetButton(Button button) const;
	float GetAbsoluteLaser(int laser) const;
	bool Are3BTsHeld() const;

	// Controller state as a string
	// Primarily used for debugging
	String GetControllerStateString() const;

	// Returns a handle to a mouse lock, release it to unlock the mouse
	MouseLockHandle LockMouse();

	// Event handlers
	virtual void OnKeyPressed(SDL_Scancode code);
	virtual void OnKeyReleased(SDL_Scancode code);
	virtual void OnMouseMotion(int32 x, int32 y);

	// Request laser input state
	float GetInputLaserDir(uint32 laserIdx);

	// Request laser input state without sensitivity applied
	float GetAbsoluteInputLaserDir(uint32 laserIdx);

	// Button delegates
	Delegate<Button> OnButtonPressed;
	Delegate<Button> OnButtonReleased;

protected:
	bool m_buttonStates[(size_t)Button::Length];
	float m_laserStates[2] = { 0.0f };

private:
	void m_InitKeyboardMapping();
	void m_InitControllerMapping();
	void m_OnButtonInput(Button b, bool pressed);

	void m_OnGamepadButtonPressed(uint8 button);
	void m_OnGamepadButtonReleased(uint8 button);

	int32 m_mouseLockIndex = 0;
	Vector<MouseLockHandle> m_mouseLocks;

	InputDevice m_laserDevice;
	InputDevice m_buttonDevice;

	bool m_backComboHold = false;
	bool m_backComboInstant = false;
	bool m_backSent = false;
	float m_rawLaserStates[2] = { 0.0f };
	float m_rawKeyLaserStates[2] = { 0.0f };
	float m_prevLaserStates[2] = { 0.0f };
	float m_absoluteLaserStates[2] = { 0.0f };
	float m_comboHoldTimer = 0.0f;

	// Keyboard bindings
	Multimap<int32, Button> m_buttonMap;
	float m_keySensitivity;
	float m_keyLaserReleaseTime;

	// Mouse bindings
	uint32 m_mouseAxisMapping[2] = { 0,1 };
	float m_mouseSensitivity;
	int32 m_lastMousePos[2];
	int32 m_mousePos[2];

	// Controller bindings
	Multimap<uint32, Button> m_controllerMap;
	uint32 m_controllerAxisMapping[2] = { 0,1 };
	float m_controllerSensitivity;
	float m_controllerDeadzone;
	bool m_controllerDirectMode;

	Ref<Gamepad> m_gamepad;

	Graphics::Window* m_window = nullptr;
};


class FakeInput : public Input {

public:
	FakeInput() {
		memset(m_buttonStates, 0, sizeof(m_buttonStates));
	}
	void UpdateButton(uint32_t b, bool pressed);
	void SetLaserValue(int ind, float val);
};
