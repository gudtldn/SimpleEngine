#include "EditorSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorSubsystem);

SE_BEGIN_REFLECT(EditorSubsystem, meta::Internal)
SE_END_REFLECT(EditorSubsystem)

bool EditorSubsystem::Initialize()
{
    return true;
}

void EditorSubsystem::Release()
{
}
}
