#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"


namespace se::editor
{
/**
 * 프로퍼티 값을 ImGui 위젯으로 렌더링하는 함수 타입
 * @param label ImGui 위젯의 라벨 (null-terminated)
 * @param value 프로퍼티 데이터의 포인터
 * @param annotations 필드에 붙은 어노테이션 (컨테이너 요소처럼 필드가 아니면 비어 있음)
 * @return 값이 수정되었으면 true
 */
using PropertyDrawFunc = bool(*)(const char* label, void* value, const AnnotationList& annotations);

/**
 * Asset Drag&Drop 시 파일 경로 -> AssetId 변환 콜백
 * @param dropped_path 드롭된 파일 경로 (null-terminated)
 * @return 변환된 AssetId (유효하지 않으면 AssetId::Invalid)
 */
using AssetDropResolverFunc = AssetId(*)(const char* dropped_path);

/**
 * TypeId별 PropertyDrawer 저장소
 *
 * 각 타입에 대한 ImGui 기반 프로퍼티 렌더링 함수를 관리합니다.
 * 내장 타입(Primitive, Math, String 등)의 Drawer는 생성 시 자동으로 등록되며,
 * Drawer가 없는 타입은 TypeInfo의 모양(struct, enum, 컨테이너, Optional)에 따라 그립니다.
 */
class SE_EDITOR_API DrawerRegistry
{
    DrawerRegistry();

public:
    static DrawerRegistry& Get();

    ~DrawerRegistry() = default;
    DrawerRegistry(const DrawerRegistry&) = delete;
    DrawerRegistry& operator=(const DrawerRegistry&) = delete;
    DrawerRegistry(DrawerRegistry&&) = delete;
    DrawerRegistry& operator=(DrawerRegistry&&) = delete;

public:
    /** 특정 타입에 대한 Drawer 함수를 등록합니다. */
    void Register(TypeId type_id, PropertyDrawFunc drawer);

    /** 특정 타입에 대한 Drawer 함수를 조회합니다. */
    [[nodiscard]] PropertyDrawFunc Find(TypeId type_id) const;

    /**
     * 구조체 타입의 부모와 필드를 ImGui 위젯으로 렌더링합니다.
     * Hidden 필드는 건너뛰고, ReadOnly 필드는 비활성(disabled) 상태로 표시됩니다.
     *
     * @param type_info 렌더링할 타입의 리플렉션 정보
     * @param instance type_info 타입의 완전 객체 포인터 (필드 오프셋의 기준)
     * @param read_only 모든 필드를 비활성 상태로 표시할지
     * @return 하나 이상의 필드가 수정되었으면 true
     */
    bool DrawProperties(const TypeInfo& type_info, void* instance, bool read_only = false);

    /**
     * 단일 값을 TypeId 기반으로 ImGui 위젯으로 렌더링합니다.
     * 등록된 Drawer가 없으면 TypeInfo의 모양에 따라 그립니다.
     *
     * @param type_id 렌더링할 값의 타입 ID
     * @param label ImGui 위젯의 라벨
     * @param value 값 데이터의 포인터
     * @param annotations 값이 필드라면 그 필드의 어노테이션
     * @param read_only 값을 비활성 상태로 표시할지 (트리는 펼칠 수 있음)
     * @return 값이 수정되었으면 true
     */
    bool DrawValue(
        TypeId type_id,
        const char* label,
        void* value,
        const AnnotationList& annotations = {},
        bool read_only = false
    );

public:
    /**
     * Asset Drag&Drop 시 경로 -> AssetId 변환 콜백을 설정합니다.
     * AssetSubsystem 등 외부 시스템에서 초기화 시 등록합니다.
     */
    void SetAssetDropResolver(AssetDropResolverFunc resolver) { asset_drop_resolver = resolver; }

    /** 현재 등록된 Asset Drop Resolver를 반환합니다. */
    [[nodiscard]] AssetDropResolverFunc GetAssetDropResolver() const { return asset_drop_resolver; }

private:
    void RegisterBuiltinDrawers();

private:
    HashMap<TypeId, PropertyDrawFunc> drawers;
    AssetDropResolverFunc asset_drop_resolver = nullptr;
};
} // namespace se::editor
