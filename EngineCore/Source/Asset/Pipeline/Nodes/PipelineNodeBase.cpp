#include "Asset/Pipeline/Nodes/PipelineNodeBase.h"


namespace se::asset
{
const Guid& PipelineNodeBase::GetUid() const noexcept
{
    return self_uid;
}

void PipelineNodeBase::SetUid(Guid new_uid) noexcept
{
    self_uid = new_uid;
}

const Guid& PipelineNodeBase::GetParentUid() const noexcept
{
    return parent_uid;
}

void PipelineNodeBase::SetParentUid(const Guid& parent)
{
    parent_uid = parent;
}

const String& PipelineNodeBase::GetDisplayName() const noexcept
{
    return display_name;
}

void PipelineNodeBase::SetDisplayName(const String& new_name) noexcept
{
    display_name = new_name;
}
} // namespace se::asset
