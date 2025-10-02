#pragma once

#include <Engine/Events/Event.h>
#include <sdl2/SDL_events.h>

namespace Spike {

	struct WindowDesc {

		std::string Name;

		uint32_t Width;
		uint32_t Height;
	};

	class Window {
	public:
		static Window* Create(const WindowDesc& desc);
		virtual ~Window() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual const std::string& GetName() const = 0;
		virtual void* GetNativeWindow() const = 0;

		virtual void Tick() = 0;

		using EventCallbackFn = std::function<void(const GenericEvent&)>;
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
	};

	// aligns with SDL_Scancode
	enum class EKey : uint8_t {

		None = 0,

		A = 4,
		B = 5,
		C = 6,
		D = 7,
		E = 8,
		F = 9,
		G = 10,
		H = 11,
		I = 12,
		J = 13,
		K = 14,
		L = 15,
		M = 16,
		N = 17,
		O = 18,
		P = 19,
		Q = 20,
		R = 21,
		S = 22,
		T = 23,
		U = 24,
		V = 25,
		W = 26,
		X = 27,
		Y = 28,
		Z = 29,

		Num1 = 30,
		Num2 = 31,
		Num3 = 32,
		Num4 = 33,
		Num5 = 34,
		Num6 = 35,
		Num7 = 36,
		Num8 = 37,
		Num9 = 38,
		Num0 = 39,

		Enter = 40,
		Esc = 41,
		Backspace = 42,
		Tab = 43,
		Space = 44,

		Right = 79,
		Left = 80,
		Down = 81,
		Up = 82,

		LCtrl = 224,
		LShift = 225,
		LAlt = 226,
		RCtrl = 228,
		RShift = 229,
		RAlt = 230
	};

	class InputHandler {
	public:
		InputHandler() {}
		~InputHandler() {}

		bool GetKeyDown(EKey key) { return m_States[key].Down; }
		bool GetKeyUp(EKey key) { return !GetKeyDown(key); }
		bool GetKeyPressed(EKey key) { return m_States[key].Pressed; }
		bool GetKeyReleased(EKey key) { return m_States[key].Released; }

		/* Note for mouse buttons :
		-right  2
		-left   0
		-middle 1 */

		bool GetMouseButtonDown(uint8_t idx) const { return m_MouseButtons[idx].Down; };
		bool GetMouseButtonUp(uint8_t idx) const { return !GetMouseButtonDown(idx); }
		bool GetMouseButtonPressed(uint8_t idx) const { return m_MouseButtons[idx].Pressed; }
		bool GetMouseButtonReleased(uint8_t idx) const { return m_MouseButtons[idx].Released; }

		int GetMouseX() const { return m_MouseX; }
		int GetMouseY() const { return m_MouseY; }
		int GetMouseDeltaX() const { return m_DeltaMouseX; }
		int GetMouseDeltaY() const { return m_DeltaMouseY; }
		int GetMouseScroll() const { return m_MouseScroll; }

		void Tick();
		void OnEvent(const SDL_Event& event);

	private:
		struct KeyState {
			bool Pressed = false;
			bool Down = false;
			bool Released = false;
		};

		std::unordered_map<EKey, KeyState> m_States;
		std::vector<EKey> m_Pressed;
		std::vector<EKey> m_Released;
		std::array<KeyState, 3> m_MouseButtons;

		int m_MouseX = 0;
		int m_MouseY = 0;
		int m_DeltaMouseX = 0;
		int m_DeltaMouseY = 0;
		int m_MouseScroll = 0;
	};

	// global input handler pointer
	extern InputHandler* GInput;
}
