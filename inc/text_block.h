#pragma once

#include "object.h"

namespace HopEngine
{

class TextBlock : public StaticMesh
{
private:
    std::string text;
    Ref<Font> font;
    glm::vec3 tint = { 0, 0, 0 };
    
public:
    DELETE_CONSTRUCTORS(TextBlock);
    TextBlock(const std::string& _text);
    
    std::string getText() const { return text; }
    void setText(const std::string& value) { text = value; updateGeometry(); }
    void setTint(const glm::vec3& value) { tint = value; updateGeometry(); }
    
	void drawImGuiDebug() override;
    
private:
    void updateGeometry();
};

}