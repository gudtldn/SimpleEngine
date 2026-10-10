#include "SimpleEditor/Asset/ImportPresetManager.h"


namespace se::editor
{
void ImportPresetManager::RegisterPreset(TypeId translator_type, Function<void(ImportProfile&)> initializer)
{
    preset_map.Insert(translator_type, std::move(initializer));
}

ImportProfile ImportPresetManager::GetDefaultProfile(TypeId translator_type) const
{
    ImportProfile profile;
    if (const auto init_fn = preset_map.Find(translator_type))
    {
        (*init_fn)(profile);
    }
    return profile;
}

bool ImportPresetManager::HasPreset(TypeId translator_type) const
{
    return preset_map.Contains(translator_type);
}
} // namespace se::editor
