#include "gtest/gtest.h"

#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"

#include <type_traits>


// canonical name 인코딩이 흔들리면 여기서 즉시 빌드가 깨집니다.
using se::detail::CanonicalNameOf;

// ─────────────────────────────────────────────────────────────
// 1. 기본형 canonical name — 컴파일러 독립적 (타입 특성 기반, __FUNCSIG__ 무관)
// ─────────────────────────────────────────────────────────────

static_assert(CanonicalNameOf<void>() == "void");
static_assert(CanonicalNameOf<bool>() == "bool");
static_assert(CanonicalNameOf<char>() == "char");

static_assert(CanonicalNameOf<signed char>()        == "i8");
static_assert(CanonicalNameOf<unsigned char>()      == "u8");
static_assert(CanonicalNameOf<int>()                == "i32");
static_assert(CanonicalNameOf<unsigned long long>() == "u64");
static_assert(CanonicalNameOf<float>()              == "f32");
static_assert(CanonicalNameOf<double>()             == "f64");

static_assert(CanonicalNameOf<i8>()  == "i8");
static_assert(CanonicalNameOf<u8>()  == "u8");
static_assert(CanonicalNameOf<i16>() == "i16");
static_assert(CanonicalNameOf<u16>() == "u16");
static_assert(CanonicalNameOf<i32>() == "i32");
static_assert(CanonicalNameOf<u32>() == "u32");
static_assert(CanonicalNameOf<f32>() == "f32");
static_assert(CanonicalNameOf<f64>() == "f64");

// 64비트 빌드에서만 검증합니다.
static_assert(sizeof(void*) != 8 || CanonicalNameOf<isize>() == "i64");
static_assert(sizeof(void*) != 8 || CanonicalNameOf<usize>() == "u64");

// cv 한정자는 postfix 이며 순서가 고정됩니다.
static_assert(CanonicalNameOf<const i32>()          == "i32 const");
static_assert(CanonicalNameOf<volatile i32>()       == "i32 volatile");
static_assert(CanonicalNameOf<const volatile i32>() == "i32 const volatile");

// 이 세 가지가 반드시 서로 달라야 합니다.
static_assert(CanonicalNameOf<const i32*>()       == "i32 const*");
static_assert(CanonicalNameOf<i32* const>()       == "i32* const");
static_assert(CanonicalNameOf<const i32* const>() == "i32 const* const");

static_assert(CanonicalNameOf<i32&>()  == "i32&");
static_assert(CanonicalNameOf<i32&&>() == "i32&&");
static_assert(CanonicalNameOf<i32**>() == "i32**");

static_assert(CanonicalNameOf<f32[4]>()    == "f32[4]");
static_assert(CanonicalNameOf<f32[4][4]>() == "f32[4][4]");
static_assert(CanonicalNameOf<f32[]>()     == "f32[]");

static_assert(CanonicalNameOf<void()>()          == "void()");
static_assert(CanonicalNameOf<void(i32, char)>() == "void(i32,char)");
static_assert(CanonicalNameOf<i32(f32*)>()       == "i32(f32*)");

// noexcept 는 함수 타입의 일부이므로 반드시 구분되어야 합니다.
static_assert(CanonicalNameOf<void() noexcept>() == "void() noexcept");
static_assert(se::TypeId::Of<void()>() != se::TypeId::Of<void() noexcept>());


// ─────────────────────────────────────────────────────────────
// 2. 엔티티(클래스/열거형) 이름 — head-cut (MSVC __FUNCSIG__) 검증
// ─────────────────────────────────────────────────────────────
namespace se_reflection_golden_test
{
struct PlainStruct {};
enum class PlainEnum { A, B };

template <typename T>
struct SimpleTemplate {};

// 비타입 인자(크기)가 섞인 템플릿 — FixedArray<T, N> 과 같은 모양입니다.
template <typename T, usize N>
struct SizedTemplate {};

// 비타입 인자만 받는 템플릿 — FixedString<N>, HashDigest<N> 과 같은 모양입니다.
template <usize N>
struct SizeOnlyTemplate {};
}

static_assert(CanonicalNameOf<se_reflection_golden_test::PlainStruct>()
    == "se_reflection_golden_test::PlainStruct");
static_assert(CanonicalNameOf<se_reflection_golden_test::PlainEnum>()
    == "se_reflection_golden_test::PlainEnum");

// 템플릿 인자 재귀 조립 — int 가 i32 로 자동 치환되는지가 head-cut 의 핵심입니다.
static_assert(CanonicalNameOf<se_reflection_golden_test::SimpleTemplate<int>>()
    == "se_reflection_golden_test::SimpleTemplate<i32>");
static_assert(CanonicalNameOf<se_reflection_golden_test::SimpleTemplate<se_reflection_golden_test::PlainStruct>>()
    == "se_reflection_golden_test::SimpleTemplate<se_reflection_golden_test::PlainStruct>");

// 비타입 인자는 10진수로 조립됩니다. 크기가 다르면 다른 타입이어야 합니다.
static_assert(CanonicalNameOf<se_reflection_golden_test::SizedTemplate<int, 4>>()
    == "se_reflection_golden_test::SizedTemplate<i32,4>");
static_assert(se::TypeId::Of<se_reflection_golden_test::SizedTemplate<i32, 4>>()
    != se::TypeId::Of<se_reflection_golden_test::SizedTemplate<i32, 8>>());

static_assert(CanonicalNameOf<se_reflection_golden_test::SizeOnlyTemplate<32>>()
    == "se_reflection_golden_test::SizeOnlyTemplate<32>");
static_assert(se::TypeId::Of<se_reflection_golden_test::SizeOnlyTemplate<32>>()
    != se::TypeId::Of<se_reflection_golden_test::SizeOnlyTemplate<64>>());


// ─────────────────────────────────────────────────────────────
// 3. TypeId
// ─────────────────────────────────────────────────────────────
static_assert(se::TypeId{}.IsNull());
static_assert(!static_cast<bool>(se::TypeId{}));
static_assert(se::TypeId::Of<i32>() == se::TypeId::Of<i32>());
static_assert(se::TypeId::Of<i32>() != se::TypeId::Of<u32>());
static_assert(se::TypeId::Of<const i32&>() == se::TypeId::Of<i32>());       // Of<T> 는 cvref 를 제거합니다.
static_assert(se::TypeId::OfExact<const i32&>() != se::TypeId::OfExact<i32>()); // OfExact<T> 는 구분합니다.


// ─────────────────────────────────────────────────────────────
// 4. TypeNameOf — 런타임 접근 경로
// ─────────────────────────────────────────────────────────────
TEST(ReflectionNameGoldenTest, TypeNameOfMatchesCanonicalName)
{
    EXPECT_EQ(se::TypeNameOf<i32>(), "i32");
    EXPECT_EQ(se::TypeNameOf<se_reflection_golden_test::PlainStruct>(), "se_reflection_golden_test::PlainStruct");
}

TEST(ReflectionNameGoldenTest, TypeIdIsStableAcrossCalls)
{
    EXPECT_EQ(se::TypeId::Of<i32>().Value(), se::TypeId::Of<i32>().Value());
    EXPECT_NE(se::TypeId::Of<i32>().Value(), se::TypeId::Of<f32>().Value());
}
