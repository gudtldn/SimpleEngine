#include "gtest/gtest.h"
#include "SimpleEngine/Core/Functional/SingleDelegate.h"
#include "SimpleEngine/Core/Functional/MultiDelegate.h"

using namespace se;

// SingleDelegate Tests
TEST(SingleDelegateTest, BindAndExecute)
{
    SingleDelegate<int(int)> delegate;
    EXPECT_FALSE(delegate.IsBound());

    delegate.BindLambda([](int i) { return i * 2; });
    EXPECT_TRUE(delegate.IsBound());

    auto result = delegate.Execute(10);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 20);
}

TEST(SingleDelegateTest, Unbind)
{
    SingleDelegate<void()> delegate;
    delegate.BindLambda([]() {});
    EXPECT_TRUE(delegate.IsBound());

    delegate.Unbind();
    EXPECT_FALSE(delegate.IsBound());
}

TEST(SingleDelegateTest, ExecuteWhenUnbound)
{
    SingleDelegate<int()> delegate;
    auto result = delegate.Execute();
    EXPECT_FALSE(result.HasValue());
}

TEST(SingleDelegateTest, Rebind)
{
    SingleDelegate<int(int)> delegate;
    delegate.BindLambda([](int i) { return i * 2; });
    delegate.BindLambda([](int i) { return i * 3; });

    auto result = delegate.Execute(10);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), 30);
}

TEST(SingleDelegateTest, VoidReturn)
{
    SingleDelegate<void(int&)> delegate;
    int value = 0;
    delegate.BindLambda([&](int& val) { val = 5; });
    delegate.Execute(value);
    EXPECT_EQ(value, 5);
}

// MultiDelegate Tests
TEST(MultiDelegateTest, Remove)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    auto handle1 = delegate.AddLambda([&](int& val) { val += 1; });
    [[maybe_unused]] auto handle2 = delegate.AddLambda([&](int& val) { val += 2; });

    delegate.Broadcast(value);
    EXPECT_EQ(value, 3);

    value = 0;
    delegate.Remove(handle1);
    delegate.Broadcast(value);
    EXPECT_EQ(value, 2);
}

TEST(MultiDelegateTest, Clear)
{
    MultiDelegate<void(int&)> delegate;
    int value = 0;

    delegate.AddLambda([&](int& val) { val += 1; });
    delegate.AddLambda([&](int& val) { val += 2; });

    delegate.Clear();
    delegate.Broadcast(value);
    EXPECT_EQ(value, 0);
}

TEST(MultiDelegateTest, BroadcastWithNoListeners)
{
    MultiDelegate<void()> delegate;
    EXPECT_NO_THROW(delegate.Broadcast());
}

TEST(MultiDelegateTest, HandleValidity)
{
    MultiDelegate<void()> delegate;
    DelegateHandle handle = delegate.AddLambda([](){});
    EXPECT_TRUE(handle.IsValid());

    delegate.Remove(handle);
    // The handle itself is not invalidated by removing it, just the binding.
    // To invalidate, we must call Invalidate()
    EXPECT_TRUE(handle.IsValid());
    handle.Invalidate();
    EXPECT_FALSE(handle.IsValid());
}
