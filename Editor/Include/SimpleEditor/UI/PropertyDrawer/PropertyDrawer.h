#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/Meta.h"


namespace se::editor
{
/**
 * 프로퍼티 값을 ImGui 위젯으로 렌더링하는 함수 타입
 * @param label ImGui 위젯의 라벨 (null-terminated)
 * @param value 프로퍼티 데이터의 포인터
 * @param prop 프로퍼티의 리플렉션 정보 (메타데이터, 타입 등)
 * @return 값이 수정되었으면 true
 */
using PropertyDrawFunc = bool(*)(const char* label, void* value, const PropertyInfo& prop);

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
 * Enum 타입은 TypeInfo::enum_entries를 통해 자동 처리됩니다.
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
    void Register(const TypeId& type_id, PropertyDrawFunc drawer);

    /** 특정 타입에 대한 Drawer 함수를 조회합니다. */
    [[nodiscard]] PropertyDrawFunc Find(const TypeId& type_id) const;

    /**
     * TypeInfo가 가진 모든 프로퍼티를 ImGui 위젯으로 렌더링합니다.
     * Hidden 프로퍼티는 건너뛰고, ReadOnly 프로퍼티는 비활성(disabled) 상태로 표시됩니다.
     * Enum 타입은 TypeInfo::enum_entries를 통해 Combo 위젯으로 자동 렌더링됩니다.
     * 등록된 Struct 타입은 TreeNode로 재귀 렌더링됩니다.
     *
     * @param type_info 렌더링할 타입의 리플렉션 정보
     * @param instance 실제 인스턴스 데이터의 포인터
     * @return 하나 이상의 프로퍼티가 수정되었으면 true
     */
    bool DrawProperties(const TypeInfo& type_info, void* instance);

    /**
     * 단일 값을 TypeId 기반으로 ImGui 위젯으로 렌더링합니다.
     * 컨테이너 요소의 재귀 렌더링에 사용됩니다.
     *
     * @param type_id 렌더링할 값의 타입 ID
     * @param label ImGui 위젯의 라벨
     * @param value 값 데이터의 포인터
     * @param container_ops 값이 컨테이너인 경우의 타입 소거 연산 (없으면 nullptr)
     * @param optional_ops 값이 Optional인 경우의 타입 소거 연산 (없으면 nullptr)
     * @return 값이 수정되었으면 true
     */
    bool DrawValue(
        const TypeId& type_id,
        const char* label,
        void* value,
        const ContainerOps* container_ops = nullptr,
        const OptionalOps* optional_ops = nullptr
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
