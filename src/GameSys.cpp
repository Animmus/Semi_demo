//
// Created by 蔡元涵 on 2020/6/2.
//

#include <glew.h>
#include <glm.hpp>
#include <algorithm>
#include "GameSys.h"
#include "Shader.h"
#include "VertexArray.h"
#include "Texture.h"
#include "SpriteComponent.h"
#include "Actor.h"
#include "Player.h"

GameSys::GameSys() : mWindow(nullptr), mContext(nullptr), mIsRunning(true), mIsUpdatingActors(
        false)
{

}

bool GameSys::InitGame()
{
    //初始化sdl和OpenGL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        SDL_Log("fail to init SDL: %s", SDL_GetError());
        return false;
    }

    //设定OpenGL属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    mWindow = SDL_CreateWindow("Semi_Demo", 100, 100,
                               1024, 768, SDL_WINDOW_OPENGL);

    if (!mWindow)
    {
        SDL_Log("fail to create window: %s", SDL_GetError());
    }

    //初始化glew
    mContext = SDL_GL_CreateContext(mWindow);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        SDL_Log("fail to init GLEW");
        return false;
    }
    glGetError();

    if (!LoadShaders())
    {
        SDL_Log("Failed to load shaders.");

        return false;
    }

    //创建纹理贴图并load actor
    CreateSpriteVerts();

    LoadData();

    mTicks = SDL_GetTicks();

    return true;
}

void GameSys::RunLoop()
{
    while (mIsRunning)
    {
        ProcessInput();
        UpdateGame();
        GenerateOutput();
    }
}

void GameSys::ProcessInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                mIsRunning = false;
                break;
            default:
                break;
        }
    }

    const Uint8 *keyState = SDL_GetKeyboardState(NULL);
    if (keyState[SDL_SCANCODE_ESCAPE])
    {
        mIsRunning = false;
    }

    mIsUpdatingActors = true;
    for (auto actor : mActors)
    {
        actor->ProcessInput(keyState);
    }
    mIsUpdatingActors = false;
}

void GameSys::UpdateGame()
{
    //强制锁定60帧
    while (!SDL_TICKS_PASSED(SDL_GetTicks(), mTicks + 16));

    mDeltaTime = (SDL_GetTicks() - mTicks) / 1000.0f;
    if (mDeltaTime > 0.05f)
    {
        mDeltaTime = 0.05f;
    }
    mTicks = SDL_GetTicks();

    //更新、移动所有actor
    mIsUpdatingActors = true;
    for (auto actor : mActors)
    {
        actor->Update(mDeltaTime);
    }
    mIsUpdatingActors = false;

    // Move any pending actors to mActors
    for (auto pending : mPendingActors)
    {
        pending->ComputeWorldTransform();
        mActors.emplace_back(pending);
    }
    mPendingActors.clear();

    // Add any dead actors to a temp vector
    std::vector<Actor *> deadActors;
    for (auto actor : mActors)
    {
        if (actor->getState() == Actor::Dead)
        {
            deadActors.emplace_back(actor);
        }
    }

    // Delete dead actors (which removes them from mActors)
    for (auto actor : deadActors)
    {
        delete actor;
    }
}

void GameSys::GenerateOutput()
{
    //背景颜色
    glClearColor(0.3f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //绘制所有精灵组件
    mSpritesShader->SetActive();
    mSpriteVerts->SetActive();
    for (auto sprite : mSprites)
    {
        sprite->Draw(mSpritesShader);
    }

    SDL_GL_SwapWindow(mWindow);
}

void GameSys::Shutdown()
{
    SDL_GL_DeleteContext(mContext);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

void GameSys::LoadData()
{
    //创建游戏actor
    mPlayer = new Player(this);
}

void GameSys::UnloadData()
{
    while (!mActors.empty())
    {
        delete mActors.back();
    }

    for (auto i:mTextures)
    {
        i.second->Unload();
        delete i.second;
    }
    mTextures.clear();
}

void GameSys::AddActor(class Actor *actor)
{
    if (mIsUpdatingActors)
    {
        mPendingActors.emplace_back(actor);
    } else
    {
        mActors.emplace_back(actor);
    }
}

void GameSys::RemoveActor(class Actor *actor)
{
    auto iter = std::find(mPendingActors.begin(), mPendingActors.end(), actor);
    if (iter != mPendingActors.end())
    {
        std::iter_swap(iter, mPendingActors.end() - 1);
        mPendingActors.pop_back();
    }

    iter = std::find(mActors.begin(), mActors.end(), actor);
    if (iter != mActors.end())
    {
        std::iter_swap(iter, mActors.end() - 1);
        mActors.pop_back();
    }
}

bool GameSys::LoadShaders()
{
    mSpritesShader = new Shader();
    if (!mSpritesShader->Load("../Shaders/Sprite.vert", "../Shaders/Sprite.frag"))
    {
        return false;
    }

    mSpritesShader->SetActive();

    glm::mat4 viewProj = glm::mat4(1.0f);
    //此处做视野转换
    viewProj[0][0] *= 2.0f / 1024.0f;
    viewProj[1][1] *= 2.0f / 768.0f;
    mSpritesShader->SetMatrixUniform("uViewProj", viewProj);

    return true;
}

void GameSys::CreateSpriteVerts()
{
    float vertices[] = {
            -0.5f, 0.5f, 0.f, 0.f, 0.f, // top left
            0.5f, 0.5f, 0.f, 1.f, 0.f, // top right
            0.5f, -0.5f, 0.f, 1.f, 1.f, // bottom right
            -0.5f, -0.5f, 0.f, 0.f, 1.f  // bottom left
    };

    unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
    };

    mSpriteVerts = new VertexArray(vertices, 4, indices, 6);
}

Texture *GameSys::GetTexture(const std::string &fileName)
{
    Texture *tex = nullptr;
    auto iter = mTextures.find(fileName);
    if (iter != mTextures.end())
    {
        tex = iter->second;
    } else
    {
        tex = new Texture();
        if (tex->Load(fileName))
        {
            mTextures.emplace(fileName, tex);
        } else
        {
            delete tex;
            tex = nullptr;
        }
    }

    return tex;
}

void GameSys::AddSprite(SpriteComponent *sprite)
{
    int myDrawOrder = sprite->getDrawOrder();
    auto iter = mSprites.begin();
    for (; iter != mSprites.end(); iter++)
    {
        if (myDrawOrder < (*iter)->getDrawOrder())
        {
            break;
        }
    }

    mSprites.insert(iter, sprite);
}

void GameSys::RemoveSprite(SpriteComponent *sprite)
{
    auto iter = std::find(mSprites.begin(), mSprites.end(), sprite);
    mSprites.erase(iter);
}