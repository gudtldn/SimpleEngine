#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"


namespace se::editor
{
/**
 * 에디터 뷰포트 카메라 상태를 표시하고 편집하는 패널
 */
class CameraPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;

protected:
    virtual void DrawContent() override;
};
} // namespace se::editor
