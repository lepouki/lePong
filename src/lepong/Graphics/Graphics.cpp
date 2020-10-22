//
// Created by lepouki on 10/12/2020.
//

#include <memory> // For std::make_unique.

#include <Windows.h>

#include "LoadOpenGLFunction.h"

#include "lepong/Assert.h"
#include "lepong/Log.h"

#include "lepong/Graphics/Graphics.h"

namespace lepong::Graphics
{

static HMODULE sOpenGLLibrary = nullptr;

bool Init() noexcept
{
    LEPONG_ASSERT_OR_RETURN_VAL(!sOpenGLLibrary, false);

    sOpenGLLibrary = LoadLibraryA("OpenGL32.dll");
    return sOpenGLLibrary;
}

void Cleanup() noexcept
{
    LEPONG_ASSERT_OR_RETURN(sOpenGLLibrary);

    FreeLibrary(sOpenGLLibrary);
    sOpenGLLibrary = nullptr;
}

///
/// I wonder what this function does.
///
LEPONG_NODISCARD static bool ShaderCompileFailed(GLuint shader) noexcept;

///
/// I love documentation.
///
static void LogShaderInfo(GLuint shader) noexcept;

GLuint CreateShaderFromSource(GLenum type, const char* source) noexcept
{
    LEPONG_ASSERT_OR_RETURN_VAL(source, 0);
    LEPONG_ASSERT_OR_RETURN_VAL(type == gl::VertexShader || type == gl::FragmentShader, 0);

    GLuint shader = gl::CreateShader(type);

    gl::ShaderSource(shader, 1, &source, nullptr);
    gl::CompileShader(shader);

    const auto kCompileFailed = ShaderCompileFailed(shader);

    if (kCompileFailed)
    {
        LogShaderInfo(shader);

        gl::DeleteShader(shader);
        shader = 0;
    }

    return shader;
}

///
/// Generic PFNGetItemiv function.
///
using PFNGetItemiv = void (*)(GLuint, GLenum, GLint*);

///
/// Returns the provided item's parameter specified by its name.
///
LEPONG_NODISCARD static GLint GetItemiv(GLuint item, GLenum name, PFNGetItemiv getItemiv) noexcept;

bool ShaderCompileFailed(GLuint shader) noexcept
{
    return !GetItemiv(shader, gl::CompileStatus, gl::GetShaderiv);
}

GLint GetItemiv(GLuint item, GLenum name, PFNGetItemiv getItemiv) noexcept
{
    GLint value;
    getItemiv(item, name, &value);
    return value;
}

///
/// Generic PFNGetItemInfo function.
///
using PFNGetItemInfo = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);

///
/// Logs the provided object's info log.
///
static void LogItemInfo(GLuint item, PFNGetItemiv getItemiv, PFNGetItemInfo getItemInfo) noexcept;

void LogShaderInfo(GLuint shader) noexcept
{
    Log::Log("Shader info:");
    LogItemInfo(shader, gl::GetShaderiv, gl::GetShaderInfoLog);
}

static void LogItemInfo(GLuint item, PFNGetItemiv getItemiv, PFNGetItemInfo getItemInfo) noexcept
{
    const auto kInfoLogLength = GetItemiv(item, gl::InfoLogLength, getItemiv);

    const auto kInfoLog = std::make_unique<GLchar[]>(kInfoLogLength);
    const auto kInfoLogData = kInfoLog.get();

    getItemInfo(item, kInfoLogLength, nullptr, kInfoLogData);
    Log::Log(kInfoLogData);
}

///
/// I wonder what this function does.
///
LEPONG_NODISCARD static bool ProgramLinkFailed(GLuint program) noexcept;

///
/// I love documentation.
///
static void LogProgramInfo(GLuint program) noexcept;

GLuint CreateProgramFromShaders(GLuint vert, GLuint frag) noexcept
{
    LEPONG_ASSERT_OR_RETURN_VAL(vert && frag, 0);

    GLuint program = gl::CreateProgram();

    gl::AttachShader(program, vert);
    gl::AttachShader(program, frag);

    gl::LinkProgram(program);

    const auto kLinkFailed = ProgramLinkFailed(program);

    if (kLinkFailed)
    {
        LogProgramInfo(program);

        gl::DeleteProgram(program);
        program = 0;
    }

    return program;
}

bool ProgramLinkFailed(GLuint program) noexcept
{
    return !GetItemiv(program, gl::LinkStatus, gl::GetProgramiv);
}

void LogProgramInfo(GLuint program) noexcept
{
    Log::Log("Program info:");
    LogItemInfo(program, gl::GetProgramiv, gl::GetProgramInfoLog);
}

PROC LoadOpenGLFunction(const char* name) noexcept
{
    LEPONG_ASSERT_OR_RETURN_VAL(sOpenGLLibrary && name, nullptr);

    const auto kFunction = wglGetProcAddress(name);

    if (!kFunction)
    {
        return GetProcAddress(sOpenGLLibrary, name);
    }

    return kFunction;
}

} // namespace lepong::Graphics
