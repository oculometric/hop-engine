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
    
	void drawImGuiDebug() override;
    void setText(const std::string& value) { text = value; updateGeometry(); }
    std::string getText() const { return text; }
    void setTint(const glm::vec3& value) { tint = value; }
    
    TextBlock(const std::string& _text);
    
private:
    void updateGeometry();
};

}