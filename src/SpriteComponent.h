//
// Created by 蔡元涵 on 2020/6/3.
//

#ifndef SEMI_DEMO_SPRITECOMPONENT_H
#define SEMI_DEMO_SPRITECOMPONENT_H

#include <SDL.h>
#include "Component.h"

class SpriteComponent : public Component
{
public:
    //draw order越小越先画
    SpriteComponent(class Object *owner, int drawOrder = 100);

    ~SpriteComponent();

    virtual void Draw(class Shader *shader);

    virtual void SetTexture(class Texture *texture);

    int getDrawOrder() const
    {
        return mDrawOrder;
    }

    int getTexHeight() const
    {
        return mTexHeight;
    }

    int getTexWidth() const
    {
        return mTexWidth;
    }

    bool getIsVisible() const
    {
        return mIsVisible;
    }

    void setIsVisible(bool mIsVisible)
    {
        SpriteComponent::mIsVisible = mIsVisible;
    }

protected:
    class Texture *mTexture;

    //是否需要渲染贴图
    bool mIsVisible;

    int mDrawOrder;
    int mTexHeight;
    int mTexWidth;
};


#endif //SEMI_DEMO_SPRITECOMPONENT_H
