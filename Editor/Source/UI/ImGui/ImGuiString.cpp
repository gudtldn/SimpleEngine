#include "UI/ImGui/ImGuiString.h"


struct InputTextCallback_UserData
{
    se::String* string;
    ImGuiInputTextCallback chain_callback;
    void* chain_callback_user_data;
};

namespace
{
int InputTextCallback(ImGuiInputTextCallbackData* data)
{
    const InputTextCallback_UserData* user_data = static_cast<InputTextCallback_UserData*>(data->UserData);
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        // 입력에 맞게 문자열 크기 조절
        se::String* str = user_data->string;
        IM_ASSERT(data->Buf == str->Data());
        str->ResizeForOverwrite(data->BufTextLen);
        data->Buf = str->Data();
    }
    else if (user_data->chain_callback)
    {
        // 원래 콜백 호출
        data->UserData = user_data->chain_callback_user_data;
        return user_data->chain_callback(data);
    }
    return 0;
}
} // namespace

namespace ImGui
{
bool InputText(const char* label, se::String* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.string = str;
    cb_user_data.chain_callback = callback;
    cb_user_data.chain_callback_user_data = user_data;
    return InputText(label, str->Data(), str->Capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

bool InputTextMultiline(const char* label, se::String* str, const ImVec2& size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.string = str;
    cb_user_data.chain_callback = callback;
    cb_user_data.chain_callback_user_data = user_data;
    return InputTextMultiline(label, str->Data(), str->Capacity() + 1, size, flags, InputTextCallback, &cb_user_data);
}

bool InputTextWithHint(const char* label, const char* hint, se::String* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
    flags |= ImGuiInputTextFlags_CallbackResize;

    InputTextCallback_UserData cb_user_data;
    cb_user_data.string = str;
    cb_user_data.chain_callback = callback;
    cb_user_data.chain_callback_user_data = user_data;
    return InputTextWithHint(label, hint, str->Data(), str->Capacity() + 1, flags, InputTextCallback, &cb_user_data);
}
} // namespace ImGui
