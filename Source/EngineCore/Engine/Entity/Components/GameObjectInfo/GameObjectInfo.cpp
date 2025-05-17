#include <Engine/Entity/Components/GameObjectInfo/GameObjectInfo.h>

namespace SpikeEngine {

	GameObjectInfo::GameObjectInfo(std::string n, std::string t, std::string l) :
		name(n), tag(t), layer(l) {}
}