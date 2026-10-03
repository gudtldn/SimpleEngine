#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"


namespace se
{
class World;

/**
 * World의 엔티티와 컴포넌트를 월드 파일(.seworld) TOML 텍스트로 씁니다.
 * 엔티티마다 영속 ID와, 타입 이름을 키로 한 컴포넌트 테이블을 씁니다. 컴포넌트의 필드는 새 리플렉션 등록을 따릅니다.
 */
class SE_CORE_API WorldFileWriter
{
public:
    /** 영속 ID가 없는 엔티티에 PersistentIdComponent를 붙여야 하므로 수정 가능한 world를 받습니다. */
    explicit WorldFileWriter(World& in_world);

    /**
     * 살아 있는 모든 엔티티를 TOML 텍스트로 만듭니다. Transient가 붙은 컴포넌트는 쓰지 않습니다.
     * 새 리플렉션에 등록되지 않은 컴포넌트나 저장하지 않는 엔티티를 가리키는 참조가 있으면 쓰지 않고 오류를 돌려줍니다.
     */
    [[nodiscard]] Expected<String, String> Write();

private:
    World& world;
};


/**
 * 월드 파일(.seworld) TOML 텍스트를 읽어 World에 엔티티를 더합니다. 이미 있는 엔티티는 그대로 둡니다.
 * 파일의 엔티티를 모두 만든 뒤 컴포넌트를 읽으므로, 뒤에 나오는 엔티티를 가리키는 참조도 풀립니다.
 */
class SE_CORE_API WorldFileReader
{
public:
    explicit WorldFileReader(World& in_world);

    /**
     * text의 엔티티를 world에 새로 만듭니다. 파일의 영속 ID가 world에 이미 있으면 새 ID를 붙이고, 파일 안의 참조는 새로 만든 엔티티를 가리킵니다.
     * 모르는 컴포넌트 타입과 파일에 없는 엔티티를 가리키는 참조는 경고로 남기고 계속 읽습니다. 실패하면 이번에 만든 엔티티를 모두 지웁니다.
     */
    [[nodiscard]] Expected<void, String> Read(StringView text);

    /**
     * 마지막 Read가 남긴 경고를 돌려줍니다.
     * 예: "WorldFileReader: entity 42: unknown component type 'se::OldComponent' is skipped."
     */
    [[nodiscard]] ArrayView<const String> GetWarnings() const;

private:
    World& world;

    /** 마지막 Read가 남긴 경고 */
    Array<String> warnings;
};
} // namespace se
