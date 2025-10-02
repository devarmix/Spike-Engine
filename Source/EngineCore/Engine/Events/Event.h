#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	enum class EEventType {

		ENone = 0, 
		EWindowResize, EWindowClose, EWindowMinimize, EWindowRestore, ESDL,
		EAssetImported
	};

	class GenericEvent {
	public:
		virtual ~GenericEvent() = default;

		virtual std::string GetName() const = 0;
		virtual EEventType GetEventType() const = 0;

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
        static Spike::EEventType GetStaticType() { return Spike::EEventType::type; } \
        Spike::EEventType GetEventType() const override { return GetStaticType(); }
