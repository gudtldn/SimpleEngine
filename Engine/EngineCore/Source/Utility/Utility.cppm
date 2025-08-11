export module SimpleEngine.Utility;

export import :FileUtils;
export import :ShaderUtils;
export import :StringUtils;

export namespace se::utility
{
#if defined(DEBUG) || defined(_DEBUG)
constexpr bool IS_DEBUG_BUILD = true;
#else
constexpr bool IS_DEBUG_BUILD = false;
#endif
}
