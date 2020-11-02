//
// Created by lepouki on 11/2/2020.
//

#include "lepong/Graphics/Quad.h"

#include "lepong/Game/Paddle.h"

namespace lepong
{

Paddle::Paddle(const Vector2f& size, float front, Graphics::Mesh& mesh, GLuint& program) noexcept
    : mMesh(mesh)
    , mProgram(program)
    , size(size)
    , front(front)
{
}

void Paddle::Render() const noexcept
{
    Graphics::DrawQuad(mMesh, size, position, mProgram);
}

void Paddle::Update(float delta, const Vector2i& winSize) noexcept
{
    const auto kPostUpdatePosition = position;

    GameObject::Update(delta);
    CollideAgainstTerrain(winSize, kPostUpdatePosition);
}

void Paddle::CollideAgainstTerrain(const Vector2i& winSize, const Vector2f& postUpdatePosition) noexcept
{
    const auto kMinTerrainOffset = size.y * 0.1f;

    const auto kCollidesTop = (position.y + kMinTerrainOffset) > (static_cast<float>(winSize.y) - size.y / 2.0f);
    const auto kCollidesBottom = (position.y - kMinTerrainOffset) < (size.y / 2.0f);

    if (kCollidesTop || kCollidesBottom)
    {
        position = postUpdatePosition;
    }
}

void Paddle::Reset(const Vector2i& winSize) noexcept
{
    position.y = winSize.y / 2.0f;

    moveSpeed = 0.0f;
    moveDirection = { 0.0f, 0.0f };
}

GLuint MakePaddleFragmentShader() noexcept
{
    constexpr auto kSource =
    R"(

    #version 330 core

    out vec4 FragColor;

    void main()
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

    )";

    return Graphics::CreateShaderFromSource(gl::FragmentShader, kSource);
}

} // namespace lepong