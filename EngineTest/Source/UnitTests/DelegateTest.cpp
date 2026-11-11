#include "gtest/gtest.h"
#include "SimpleEngine/Core/Functional/SingleDelegate.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"

#include <memory>

using namespace se;

// SingleDelegate Tests
TEST(SingleDelegateTest, BindAndExecute)
{
    SingleDelegate<int(int)> delegate;
    EXPECT_FALSE(delegate.IsBound());

    ScopedDelegateHandle handle = delegate.Bind([](int i) { return i * 2; });
    EXPECT_TRUE(delegate.IsBound());
    EXPECT_TRUE(handle.IsValid());

    auto result = delegate.Execute(10);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 20);
}

TEST(SingleDelegateTest, ExecuteWithLvalueArgument)
{
    SingleDelegate<int(int)> delegate;
    ScopedDelegateHandle handle = delegate.Bind([](int i) { return i + 1; });

    int value = 41;
    auto result = delegate.Execute(value);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 42);
}

TEST(SingleDelegateTest, Clear)
{
    SingleDelegate<void()> delegate;
    ScopedDelegateHandle handle = delegate.Bind([] {});
    EXPECT_TRUE(delegate.IsBound());

    delegate.Clear();
    EXPECT_FALSE(delegate.IsBound());
    EXPECT_FALSE(handle.IsValid());
}

TEST(SingleDelegateTest, ExecuteWhenUnbound)
{
    SingleDelegate<int()> delegate;
    auto result = delegate.Execute();
    EXPECT_FALSE(result.HasValue());
}

TEST(SingleDelegateTest, RebindInvalidatesOldHandle)
{
    SingleDelegate<int(int)> delegate;
    ScopedDelegateHandle old_handle = delegate.Bind([](int i) { return i * 2; });
    ScopedDelegateHandle new_handle = delegate.Bind([](int i) { return i * 3; });

    EXPECT_FALSE(old_handle.IsValid());
    EXPECT_TRUE(new_handle.IsValid());

    // 비활성 핸들을 Reset해도 새 바인딩은 남아야 합니다.
    old_handle.Reset();
    auto result = delegate.Execute(10);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 30);
}

TEST(SingleDelegateTest, VoidReturn)
{
    SingleDelegate<void(int&)> delegate;
    int value = 0;
    ScopedDelegateHandle handle = delegate.Bind([](int& val) { val = 5; });
    delegate.Execute(value);
    EXPECT_EQ(value, 5);
}

TEST(SingleDelegateTest, HandleDestructionUnbinds)
{
    SingleDelegate<void()> delegate;
    {
        ScopedDelegateHandle handle = delegate.Bind([] {});
        EXPECT_TRUE(delegate.IsBound());
    }
    EXPECT_FALSE(delegate.IsBound());
}

TEST(SingleDelegateTest, DetachKeepsBinding)
{
    SingleDelegate<int()> delegate;
    {
        (void)delegate.Bind([] { return 7; }).Detach();
    }
    auto result = delegate.Execute();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 7);
}

TEST(SingleDelegateTest, ClearDuringExecuteIsDeferred)
{
    SingleDelegate<int()> delegate;
    ScopedDelegateHandle handle = delegate.Bind([&delegate]
    {
        delegate.Clear();
        return 1; // Clear 뒤에도 실행 중인 람다는 끝까지 안전해야 합니다.
    });

    auto result = delegate.Execute();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 1);
    EXPECT_FALSE(delegate.IsBound());
}

TEST(SingleDelegateTest, RebindDuringExecuteIsDeferred)
{
    SingleDelegate<int()> delegate;
    ScopedDelegateHandle second_handle;
    ScopedDelegateHandle first_handle = delegate.Bind([&]
    {
        second_handle = delegate.Bind([] { return 2; });
        return 1;
    });

    auto first = delegate.Execute();
    ASSERT_TRUE(first.HasValue());
    EXPECT_EQ(first.Value(), 1);
    EXPECT_FALSE(first_handle.IsValid());
    EXPECT_TRUE(second_handle.IsValid());

    auto second = delegate.Execute();
    ASSERT_TRUE(second.HasValue());
    EXPECT_EQ(second.Value(), 2);
}

TEST(SingleDelegateTest, DelegateDestroyedFirstInvalidatesHandle)
{
    ScopedDelegateHandle handle;
    {
        SingleDelegate<void()> delegate;
        handle = delegate.Bind([] {});
        EXPECT_TRUE(handle.IsValid());
    }
    EXPECT_FALSE(handle.IsValid());
    handle.Reset();
}

TEST(SingleDelegateTest, MoveOnlyCapture)
{
    SingleDelegate<int()> delegate;
    auto value = std::make_unique<int>(9);
    ScopedDelegateHandle handle = delegate.Bind([captured = std::move(value)] { return *captured; });

    auto result = delegate.Execute();
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 9);
}

// MultiDelegate Tests
TEST(MultiDelegateTest, AddAndBroadcast)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle handle1 = delegate.Add([](int& val) { val += 1; });
    ScopedDelegateHandle handle2 = delegate.Add([](int& val) { val += 2; });

    delegate.Broadcast(value);
    EXPECT_EQ(value, 3);
}

TEST(MultiDelegateTest, BroadcastWithNoListeners)
{
    MultiDelegate<void()> delegate;
    EXPECT_FALSE(delegate.IsBound());
    EXPECT_NO_THROW(delegate.Broadcast());
}

TEST(MultiDelegateTest, HandleDestructionRemoves)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle kept = delegate.Add([](int& val) { val += 1; });
    {
        ScopedDelegateHandle temporary = delegate.Add([](int& val) { val += 2; });
    }

    delegate.Broadcast(value);
    EXPECT_EQ(value, 1);
}

TEST(MultiDelegateTest, ResetRemovesOnlyThatSubscription)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle handle1 = delegate.Add([](int& val) { val += 1; });
    ScopedDelegateHandle handle2 = delegate.Add([](int& val) { val += 10; });
    ScopedDelegateHandle handle3 = delegate.Add([](int& val) { val += 100; });

    // 첫 슬롯을 지우면 마지막 슬롯이 그 자리로 옮겨집니다. 옮겨진 handle3이 자기 슬롯을 지워야 합니다.
    handle1.Reset();
    handle3.Reset();

    delegate.Broadcast(value);
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(handle2.IsValid());
}

TEST(MultiDelegateTest, DetachAndRemove)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    const DelegateHandle handle = delegate.Add([](int& val) { val += 1; }).Detach();
    EXPECT_TRUE(handle.IsValid());

    delegate.Broadcast(value);
    EXPECT_EQ(value, 1);

    EXPECT_TRUE(delegate.Remove(handle));
    EXPECT_FALSE(delegate.Remove(handle));

    delegate.Broadcast(value);
    EXPECT_EQ(value, 1);
}

TEST(MultiDelegateTest, RemoveWithHandleFromOtherDelegateFails)
{
    MultiDelegate<void()> delegate_a;
    MultiDelegate<void()> delegate_b;

    const DelegateHandle handle_a = delegate_a.Add([] {}).Detach();
    (void)delegate_b.Add([] {}).Detach();

    EXPECT_FALSE(delegate_b.Remove(handle_a));
    EXPECT_TRUE(delegate_b.IsBound());
}

TEST(MultiDelegateTest, Clear)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle handle1 = delegate.Add([](int& val) { val += 1; });
    ScopedDelegateHandle handle2 = delegate.Add([](int& val) { val += 2; });

    delegate.Clear();
    EXPECT_FALSE(delegate.IsBound());
    EXPECT_FALSE(handle1.IsValid());
    EXPECT_FALSE(handle2.IsValid());

    delegate.Broadcast(value);
    EXPECT_EQ(value, 0);
}

TEST(MultiDelegateTest, ResetSelfDuringBroadcastIsDeferred)
{
    MultiDelegate<void()> delegate;
    int call_count = 0;

    ScopedDelegateHandle handle;
    handle = delegate.Add([&]
    {
        ++call_count;
        handle.Reset();
    });

    delegate.Broadcast();
    delegate.Broadcast();
    EXPECT_EQ(call_count, 1);
    EXPECT_FALSE(delegate.IsBound());
}

TEST(MultiDelegateTest, AddDuringBroadcastIsDeferred)
{
    MultiDelegate<void()> delegate;
    int added_call_count = 0;

    ScopedDelegateHandle added_handle;
    ScopedDelegateHandle handle = delegate.Add([&]
    {
        if (!added_handle.IsValid())
        {
            added_handle = delegate.Add([&] { ++added_call_count; });
        }
    });

    delegate.Broadcast();
    EXPECT_EQ(added_call_count, 0);

    delegate.Broadcast();
    EXPECT_EQ(added_call_count, 1);
}

TEST(MultiDelegateTest, AddThenResetDuringBroadcast)
{
    MultiDelegate<void()> delegate;
    int added_call_count = 0;

    ScopedDelegateHandle handle = delegate.Add([&]
    {
        ScopedDelegateHandle added = delegate.Add([&] { ++added_call_count; });
        added.Reset();
    });

    delegate.Broadcast();
    delegate.Broadcast();
    EXPECT_EQ(added_call_count, 0);
}

TEST(MultiDelegateTest, ClearDuringBroadcastIsDeferred)
{
    MultiDelegate<void()> delegate;
    int call_count = 0;

    ScopedDelegateHandle handle1 = delegate.Add([&]
    {
        ++call_count;
        delegate.Clear();
    });
    ScopedDelegateHandle handle2 = delegate.Add([&]
    {
        ++call_count;
        delegate.Clear();
    });

    // 먼저 불린 쪽이 Clear하므로 이번 호출에서는 하나만 불립니다.
    delegate.Broadcast();
    EXPECT_EQ(call_count, 1);
    EXPECT_FALSE(delegate.IsBound());

    delegate.Broadcast();
    EXPECT_EQ(call_count, 1);
}

TEST(MultiDelegateTest, NestedBroadcast)
{
    MultiDelegate<void(int)> delegate;
    int total = 0;

    ScopedDelegateHandle handle = delegate.Add([&](int depth)
    {
        total += 1;
        if (depth > 0)
        {
            delegate.Broadcast(depth - 1);
        }
    });

    delegate.Broadcast(2);
    EXPECT_EQ(total, 3);
}

TEST(MultiDelegateTest, HandleMoveKeepsSubscription)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle original = delegate.Add([](int& val) { val += 1; });
    ScopedDelegateHandle moved = std::move(original);
    EXPECT_FALSE(original.IsValid()); // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE(moved.IsValid());

    delegate.Broadcast(value);
    EXPECT_EQ(value, 1);

    moved.Reset();
    delegate.Broadcast(value);
    EXPECT_EQ(value, 1);
}

TEST(MultiDelegateTest, HandleMoveAssignReleasesPreviousSubscription)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    ScopedDelegateHandle handle = delegate.Add([](int& val) { val += 1; });
    handle = delegate.Add([](int& val) { val += 10; });

    delegate.Broadcast(value);
    EXPECT_EQ(value, 10);
}

TEST(MultiDelegateTest, DelegateDestroyedFirstInvalidatesHandle)
{
    ScopedDelegateHandle handle;
    {
        MultiDelegate<void()> delegate;
        handle = delegate.Add([] {});
        EXPECT_TRUE(handle.IsValid());
    }
    EXPECT_FALSE(handle.IsValid());
    handle.Reset();
}

TEST(MultiDelegateTest, DelegateMoveKeepsHandles)
{
    MultiDelegate<void(int&)> source;
    int value = 0;

    ScopedDelegateHandle handle = source.Add([](int& val) { val += 1; });
    MultiDelegate<void(int&)> target = std::move(source);

    target.Broadcast(value);
    EXPECT_EQ(value, 1);

    handle.Reset();
    target.Broadcast(value);
    EXPECT_EQ(value, 1);
}
