#include "SimpleEngine/Utility/SubsystemUtils.h"
#include "SimpleEngine/Core/Engine/Engine.h"


namespace se
{
SubsystemBase* GetSubsystem(const TypeId& type_id)
{
    return Engine::Get().GetSubsystem(type_id);
}
} // namespace se
