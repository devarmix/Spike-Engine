#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	enum EventType {

		NONE = 0, WindowResize, WindowClose, WindowMinimize, WindowRestore, 
		SDL, SceneViewportResize, SceneViewportMinimize, SceneViewportRestore,
		KeyPress, KeyRelease, KeyRepeat,
		MouseButtonPress, MouseButtonRelease, MouseMotion, MouseScroll
	};

	class Event {
	public:
		virtual ~Event() = default;

		virtual std::string GetName() const = 0;

		virtual EventType GetEventType() const = 0;

		bool IsHandled = false;
	};

	class EventDispatcher {
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		bool Dispatch(const F& func) {

			if (m_Event.GetEventType() == T::GetStaticType()) {

				m_Event.IsHandled = func(static_cast<T&>(m_Event));
				return true;
			}

			return false;
		}

	private:
		Event& m_Event;
	};
}

#define EVENT_CLASS_TYPE(type) \
        static Spike::EventType GetStaticType() { return Spike::EventType::type; } \
        Spike::EventType GetEventType() const { return GetStaticType(); }

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)
