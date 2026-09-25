#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <cstring>
#include <variant>


namespace se
{
// 노드 호출 하나하나를 나타내는 이벤트들입니다. TypeShape의 형태 목록과 같은 방식으로,
// 같은 모양(예: Int/Enum)이라도 서로 다른 노드로 구분되도록 별도 타입을 둡니다.
struct EventInt { i64 value; EIntWidth width; bool is_signed; };
struct EventFloat { f64 value; EFloatWidth width; };
struct EventBool { bool value; };
struct EventStr { String value; };
struct EventBytes { Array<u8> value; };
struct EventEnum { i64 value; EIntWidth width; bool is_signed; };
struct EventBeginStruct {};
struct EventField { String name; };
struct EventEndStruct {};
struct EventBeginSeq { u64 count; ESeqOrder order; };
struct EventEndSeq {};
struct EventBeginMap { u64 count; };
struct EventBeginMapEntry {};
struct EventEndMapEntry {};
struct EventEndMap {};
struct EventPresent { bool value; };

/** EventWriter가 기록하고 EventReader가 재생하는 노드 호출 하나 */
using SerializeEvent = std::variant<
    EventInt, EventFloat, EventBool, EventStr, EventBytes, EventEnum,
    EventBeginStruct, EventField, EventEndStruct,
    EventBeginSeq, EventEndSeq,
    EventBeginMap, EventBeginMapEntry, EventEndMapEntry, EventEndMap,
    EventPresent
>;

/** 에러 메시지용으로 이벤트의 노드 종류 이름을 반환합니다. */
[[nodiscard]] inline StringView GetEventKindName(const SerializeEvent& event)
{
    return std::visit(Overloaded{
        [](const EventInt&)           -> StringView { return "Int"; },
        [](const EventFloat&)         -> StringView { return "Float"; },
        [](const EventBool&)          -> StringView { return "Bool"; },
        [](const EventStr&)           -> StringView { return "Str"; },
        [](const EventBytes&)         -> StringView { return "Bytes"; },
        [](const EventEnum&)          -> StringView { return "Enum"; },
        [](const EventBeginStruct&)   -> StringView { return "BeginStruct"; },
        [](const EventField&)         -> StringView { return "Field"; },
        [](const EventEndStruct&)     -> StringView { return "EndStruct"; },
        [](const EventBeginSeq&)      -> StringView { return "BeginSeq"; },
        [](const EventEndSeq&)        -> StringView { return "EndSeq"; },
        [](const EventBeginMap&)      -> StringView { return "BeginMap"; },
        [](const EventBeginMapEntry&) -> StringView { return "BeginMapEntry"; },
        [](const EventEndMapEntry&)   -> StringView { return "EndMapEntry"; },
        [](const EventEndMap&)        -> StringView { return "EndMap"; },
        [](const EventPresent&)       -> StringView { return "Present"; },
    }, event);
}

/** 노드 호출을 이벤트 배열로 기록하는 ArchiveWriter. 텍스트 분기를 TOML 없이 왕복 테스트할 때 씁니다. */
class EventWriter final : public ArchiveWriter
{
public:
    EventWriter(Array<SerializeEvent>& out_events, bool is_text_format)
        : events(out_events)
        , text_format(is_text_format)
    {
    }

public:
    [[nodiscard]] virtual bool IsTextFormat() const override { return text_format; }

    virtual void Int(i64 value, EIntWidth width, bool is_signed) override
    {
        Record(SerializeEvent{ EventInt{ value, width, is_signed } });
    }

    virtual void Float(f64 value, EFloatWidth width) override
    {
        Record(SerializeEvent{ EventFloat{ value, width } });
    }

    virtual void Bool(bool value) override
    {
        Record(SerializeEvent{ EventBool{ value } });
    }

    virtual void Str(StringView value) override
    {
        Record(SerializeEvent{ EventStr{ String(value) } });
    }

    virtual void Bytes(const void* data, u64 size) override
    {
        Array<u8> copy = Array<u8>::Uninitialized(static_cast<usize>(size));
        std::memcpy(copy.Data(), data, size);
        Record(SerializeEvent{ EventBytes{ std::move(copy) } });
    }

    virtual void Enum(i64 value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries) override
    {
        Record(SerializeEvent{ EventEnum{ value, width, is_signed } });
    }

    virtual void BeginStruct() override
    {
        Record(SerializeEvent{ EventBeginStruct{} });
    }

    virtual void Field(StringView name) override
    {
        Record(SerializeEvent{ EventField{ String(name) } });
    }

    virtual void EndStruct() override
    {
        Record(SerializeEvent{ EventEndStruct{} });
    }

    virtual void BeginSeq(u64 count, ESeqOrder order) override
    {
        Record(SerializeEvent{ EventBeginSeq{ count, order } });
    }

    virtual void EndSeq() override
    {
        Record(SerializeEvent{ EventEndSeq{} });
    }

    virtual void BeginMap(u64 count) override
    {
        Record(SerializeEvent{ EventBeginMap{ count } });
    }

    virtual void BeginMapEntry() override
    {
        Record(SerializeEvent{ EventBeginMapEntry{} });
    }

    virtual void EndMapEntry() override
    {
        Record(SerializeEvent{ EventEndMapEntry{} });
    }

    virtual void EndMap() override
    {
        Record(SerializeEvent{ EventEndMap{} });
    }

    virtual void Present(bool has_value) override
    {
        Record(SerializeEvent{ EventPresent{ has_value } });
    }

private:
    /** 오류가 이미 켜져 있으면 기록하지 않습니다(Archive 계약: 오류 이후 연산은 no-op). */
    void Record(SerializeEvent event)
    {
        if (HasError())
        {
            return;
        }
        events.Push(std::move(event));
    }

private:
    Array<SerializeEvent>& events;
    bool text_format;
};

/**
 * 이벤트 배열을 순서대로 재생하는 ArchiveReader
 * 다음 이벤트의 종류가 기대와 다르면 에러를 남깁니다. 단, Field는 예외로,
 * 이름이 다르거나 다음 이벤트가 Field가 아니면 에러 없이 false를 돌려줘 필드 누락을 흉내냅니다.
 */
class EventReader final : public ArchiveReader
{
public:
    EventReader(ArrayView<const SerializeEvent> in_events, bool is_text_format)
        : events(in_events)
        , text_format(is_text_format)
    {
    }

public:
    [[nodiscard]] virtual bool IsTextFormat() const override { return text_format; }

    virtual void Int(i64& value, EIntWidth width, bool is_signed) override
    {
        const EventInt* data = Consume<EventInt>();
        if (data == nullptr)
        {
            return;
        }
        if (data->width != width || data->is_signed != is_signed)
        {
            SetError("EventReader: Int width/is_signed does not match the recorded event.");
            return;
        }
        value = data->value;
    }

    virtual void Float(f64& value, EFloatWidth width) override
    {
        const EventFloat* data = Consume<EventFloat>();
        if (data == nullptr)
        {
            return;
        }
        if (data->width != width)
        {
            SetError("EventReader: Float width does not match the recorded event.");
            return;
        }
        value = data->value;
    }

    virtual void Bool(bool& value) override
    {
        if (const EventBool* data = Consume<EventBool>())
        {
            value = data->value;
        }
    }

    virtual void Str(String& value) override
    {
        if (const EventStr* data = Consume<EventStr>())
        {
            value = data->value;
        }
    }

    virtual void Bytes(void* data, u64 size) override
    {
        const EventBytes* event = Consume<EventBytes>();
        if (event == nullptr)
        {
            return;
        }
        if (event->value.Len() != size)
        {
            SetError(String::Format("EventReader: Bytes size mismatch (expected {}, got {}).", size, event->value.Len()));
            return;
        }
        std::memcpy(data, event->value.Data(), size);
    }

    virtual void Enum(i64& value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries) override
    {
        const EventEnum* data = Consume<EventEnum>();
        if (data == nullptr)
        {
            return;
        }
        if (data->width != width || data->is_signed != is_signed)
        {
            SetError("EventReader: Enum width/is_signed does not match the recorded event.");
            return;
        }
        value = data->value;
    }

    virtual void BeginStruct() override
    {
        Consume<EventBeginStruct>();
    }

    [[nodiscard]] virtual bool Field(StringView name) override
    {
        if (HasError() || cursor >= events.Len())
        {
            return false;
        }
        const EventField* data = std::get_if<EventField>(&events[cursor]);
        if (data == nullptr || !(StringView{ data->name } == name))
        {
            return false;
        }
        ++cursor;
        return true;
    }

    virtual void EndStruct() override
    {
        Consume<EventEndStruct>();
    }

    virtual void BeginSeq(u64& count) override
    {
        if (const EventBeginSeq* data = Consume<EventBeginSeq>())
        {
            count = data->count;
        }
    }

    virtual void EndSeq() override
    {
        Consume<EventEndSeq>();
    }

    virtual void BeginMap(u64& count) override
    {
        if (const EventBeginMap* data = Consume<EventBeginMap>())
        {
            count = data->count;
        }
    }

    virtual void BeginMapEntry() override
    {
        Consume<EventBeginMapEntry>();
    }

    virtual void EndMapEntry() override
    {
        Consume<EventEndMapEntry>();
    }

    virtual void EndMap() override
    {
        Consume<EventEndMap>();
    }

    virtual void Present(bool& has_value) override
    {
        if (const EventPresent* data = Consume<EventPresent>())
        {
            has_value = data->value;
        }
    }

private:
    /** 다음 이벤트가 EventT이면 소비하고 그 payload를 돌려줍니다. 아니면 SetError 후 nullptr입니다. */
    template <typename EventT>
    const EventT* Consume()
    {
        if (HasError())
        {
            return nullptr;
        }
        if (cursor >= events.Len())
        {
            SetError(String::Format("EventReader: expected {}, got end of stream.", GetEventKindName(SerializeEvent{ EventT{} })));
            return nullptr;
        }

        const SerializeEvent& event = events[cursor];
        const EventT* found = std::get_if<EventT>(&event);
        if (found == nullptr)
        {
            SetError(String::Format(
                "EventReader: expected {}, got {}.",
                GetEventKindName(SerializeEvent{ EventT{} }), GetEventKindName(event)
            ));
            return nullptr;
        }

        ++cursor;
        return found;
    }

private:
    ArrayView<const SerializeEvent> events;
    bool text_format;
    usize cursor = 0;
};
} // namespace se
