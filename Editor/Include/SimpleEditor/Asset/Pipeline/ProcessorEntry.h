#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::editor
{
/**
 * 파이프라인에서 실행될 개별 Processor의 직렬화 가능한 데이터 Wrapper
 */
struct ProcessorEntry
{
    /** Processor의 구체 타입 (예: TypeId::Of<StaticMeshOptimizer>()) */
    TypeId processor_type;

    /** 파이프라인 실행 시 이 Processor를 건너뛸 여부 */
    bool enabled = true;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::ProcessorEntry, SE_EDITOR_API)
