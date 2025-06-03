#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	enum EventType {

		NONE = 0, 
		WindowResize, WindowClose, WindowMinimize, WindowRestore, 
		SDL, SceneViewportResize, SceneViewportMinimize, SceneViewportRestore,
		KeyPress, KeyRelease, KeyRepeat,
		MouseButtonPress, MouseButtonRelease, MouseMotion, MouseScroll
	};

	class GenericEvent {
	public:
		virtual ~GenericEvent() = default;

		virtual std::string GetName() const = 0;

		virtual EventType GetEventType() const = 0;

		void SetHandled() const { m_IsHandled = true; }
		bool IsHandled() const { return m_IsHandled; }

	private:
		 mutable bool m_IsHandled = false;
	};

	class EventHandler {
	public:
		EventHandler(const GenericEvent& event) : m_Event(event) {}

		template<typename T>
		using EventFN = std::function<bool(const T&)>;

		template<typename T>
		bool Handle(const EventFN<T>& func) {

			if (m_Event.GetEventType() == T::GetStaticType()) {

				if (func((const T&)m_Event)) m_Event.SetHandled();
				return true;
			}

			return false;
		}

	private:
		const GenericEvent& m_Event;
	};
}

#define EVENT_CLASS_TYPE(type) \
        static Spike::EventType GetStaticType() { return Spike::EventType::type; } \
        Spike::EventType GetEventType() const override { return GetStaticType(); }
