#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::editor
{
/**
 * 파이프라인에서 실행될 개별 Processor의 직렬화 가능한 데이터 Wrapper
 */
struct SE_ANNOTATION(=meta::SerializeOnly) ProcessorEntry
{
    /** Processor의 구체 타입 (예: TypeId::Of<StaticMeshOptimizer>()) */
    SE_ANNOTATION(=meta::Property)
    TypeId processor_type;

    /** 파이프라인 실행 시 이 Processor를 건너뛸 여부 */
    SE_ANNOTATION(=meta::Property)
    bool enabled = true;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::ProcessorEntry);
