//
// Created by lepouki on 11/2/2020.
//

#pragma once

#include "lepong/Graphics/GL.h"
#include "lepong/Graphics/Mesh.h"

#include "GameObject.h"

namespace lepong
{

class Paddle : public GameObject
{
public:
    static constexpr auto skDefaultMoveSpeed = 300.0f;

public:
    Vector2f size;

    // The x direction the paddle is facing.
    float forward;

public:
    Paddle(const Vector2f& size, float forward, Graphics::Mesh& mesh, GLuint& program) noexcept;

public:
    void Update(float delta, const Vector2i& winSize) noexcept;
    void Render() const noexcept;

public:
    ///
    /// Resets the paddle to its default state.
    ///
    void Reset(const Vector2i& winSize) noexcept;

public:
    void OnMoveUpPressed() noexcept;
    void OnMoveDownPressed() noexcept;
    void OnMoveUpReleased() noexcept;
    void OnMoveDownReleased() noexcept;

private:
    Graphics::Mesh& mMesh;
    GLuint& mProgram;

private:
    void CollideWithTerrain(const Vector2i& winSize, const Vector2f& preUpdatePosition) noexcept;
};

///
/// A basic fragment shader that outputs white.
///
LEPONG_NODISCARD GLuint MakePaddleFragmentShader() noexcept;

} // namespace lepong
