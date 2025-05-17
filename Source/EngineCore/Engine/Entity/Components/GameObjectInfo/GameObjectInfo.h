#pragma once

#include <Engine/Core/Core.h>

namespace SpikeEngine {

	class GameObject;

	class GameObjectInfo {
	public:
		GameObjectInfo(std::string n, std::string t, std::string l);

		std::string name;
		std::string tag;
		std::string layer;

		std::weak_ptr<GameObject> parent;
		std::vector<Ref<GameObject>> children;
	};
}
