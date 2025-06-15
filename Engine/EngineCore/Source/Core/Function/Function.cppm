export module SimpleEngine.Core.Function;


template <typename Signature>
struct TFunction;

export template <typename ReturnType, typename... ParamsType>
struct TFunction<ReturnType(ParamsType...)>
{
};
