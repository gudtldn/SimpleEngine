#include "EditorSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(EditorSubsystem);

bool EditorSubsystem::Initialize()
{
    return true;
}

void EditorSubsystem::Release()
{
}
}
