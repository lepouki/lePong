//
// Created by lepouki on 10/12/2020.
//

#include <cstdint>
#include <ctime>

#include "lepong/Check.h"
#include "lepong/lepong.h"
#include "lepong/Window.h"
#include "lepong/Game/Game.h"
#include "lepong/Graphics/Quad.h"
#include "lepong/Math/Math.h"
#include "lepong/Time/Time.h"

namespace lepong
{

// Hardcoded but this should be fine on most monitors (maybe a bit small for 2k+).
static constexpr Vector2i skWinSize = { 1280, 720 };

static auto sInitialized = false;

static HWND sWindow;
static gl::Context sContext;

static GLuint sPaddleProgram;
static GLuint sBallProgram;

static Graphics::Mesh sQuad;
static Graphics::Mesh sTexturedQuad;

// Game state.
static bool sPlaying = false;
static unsigned sPlayerScores[] = { 0u, 0u };

// Ball.
static constexpr float skBallRadius = 20.0f;

static Ball sBall{ skBallRadius, sTexturedQuad, sBallProgram };

// Paddles.
static constexpr Vector2f skPaddleSize = { 25.0f, 150.0f };

static Paddle sPaddle1{ skPaddleSize,  1.0f, sQuad, sPaddleProgram };
static Paddle sPaddle2{ skPaddleSize, -1.0f, sQuad, sPaddleProgram };

///
/// A class holding the init and cleanup functions of any item.
///
class Lifetime
{
public:
    ///
    /// A generic initialization function.<br>
    /// Initialization functions must return false on failure.
    ///
    using PFNInit = bool (*)();

    ///
    /// A generic cleanup function.
    ///
    using PFNCleanup = void (*)();

public:
    ///
    /// Both init and cleanup functions must be provided.
    ///
    constexpr Lifetime(PFNInit init, PFNCleanup cleanup) noexcept
        : mInit(init), mCleanup(cleanup)
    {
    }

public:
    ///
    /// Pretty straight forward.
    ///
    /// \return Whether the item was successfully initialized.
    ///
    LEPONG_NODISCARD constexpr bool Init() const noexcept
    {
        return mInit();
    }

    ///
    /// Pretty straight forward.
    ///
    constexpr void Cleanup() const noexcept
    {
        mCleanup();
    }

private:
    PFNInit mInit;
    PFNCleanup mCleanup;
};

///
/// Initializes the game's systems.
///
LEPONG_NODISCARD static bool InitGameSystems() noexcept;

///
/// Cleans up the game's systems.
///
static void CleanupGameSystems() noexcept;

///
/// Initializes the game's state and resources.
///
LEPONG_NODISCARD static bool InitState() noexcept;

///
/// Cleans up the game's state and resources.
///
static void CleanupState() noexcept;

///
/// All the game's lifetimes.
///
static constexpr Lifetime kGameLifetimes[] =
{
    { InitGameSystems, CleanupGameSystems },
    { InitState, CleanupState }
};

///
/// A wrapper for the pretty ugly array reference syntax.
///
template<typename T, std::size_t Size>
using ConstArrayReference = const T (&)[Size];

///
/// Tries to run all the provided lifetime initialization functions.<br>
/// If an initialization function fails, the cleanup functions of the initialized items are called in reverse order.
///
/// \return Whether all the items have been successfully initialized.
///
template<std::size_t NumItems>
LEPONG_NODISCARD static bool TryInitItems(ConstArrayReference<Lifetime, NumItems> lifetimes) noexcept;

bool Init() noexcept
{
    LEPONG_CHECK_OR_RETURN_VAL(!sInitialized, false);

    sInitialized = TryInitItems(kGameLifetimes);
    return sInitialized;
}

///
/// Cleans up all the items starting from the item at the provided index.<br>
/// Items are cleaned up in reverse order.
///
/// \param index The index of the first system to cleanup.
///
template<std::size_t NumItems>
static void CleanupItemsStartingAt(ConstArrayReference<Lifetime, NumItems> lifetimes, unsigned index) noexcept;

template<std::size_t NumItems>
bool TryInitItems(ConstArrayReference<Lifetime, NumItems> lifetimes) noexcept
{
    auto succeeded = true;

    for (unsigned i = 0; succeeded && i < NumItems; ++i)
    {
        succeeded = lifetimes[i].Init();

        if (!succeeded)
        {
            CleanupItemsStartingAt(lifetimes, i);
        }
    }

    return succeeded;
}

template<std::size_t NumItems>
void CleanupItemsStartingAt(ConstArrayReference<Lifetime, NumItems> lifetimes, unsigned index) noexcept
{
    for (auto i = static_cast<int>(index) - 1; i >= 0; --i)
    {
        lifetimes[i].Cleanup();
    }
}

///
/// All the system lifetimes.
///
static constexpr Lifetime skSystemLifetimes[] =
{
    { Log::Init, Log::Cleanup },
    { Window::Init, Window::Cleanup },
    { Graphics::Init, Graphics::Cleanup },
    { gl::Init, gl::Cleanup },
    { Time::Init, Time::Cleanup }
};

bool InitGameSystems() noexcept
{
    return TryInitItems(skSystemLifetimes);
}

///
/// Calls the cleanup function of all the provided item lifetimes.
///
template<std::size_t NumItems>
static void CleanupItems(ConstArrayReference<Lifetime, NumItems> itemLifetimes) noexcept;

void CleanupGameSystems() noexcept
{
    CleanupItems(skSystemLifetimes);
}

template<std::size_t NumItems>
void CleanupItems(ConstArrayReference<Lifetime, NumItems> itemLifetimes) noexcept
{
    CleanupItemsStartingAt(itemLifetimes, NumItems);
}

///
/// \return Whether the game window was successfully initialized.
///
LEPONG_NODISCARD static bool InitGameWindow() noexcept;

///
/// Cleans up the game window.
///
static void CleanupGameWindow() noexcept;

///
/// \return Whether the rendering context was successfully initialized.
///
LEPONG_NODISCARD static bool InitContext() noexcept;

///
/// Cleans up the rendering context.
///
static void CleanupContext() noexcept;

///
/// \return Whether the graphics resources were successfully initialized.
///
LEPONG_NODISCARD static bool InitGraphicsResources() noexcept;

///
/// Cleans up the graphics resources.
///
static void CleanupGraphicsResources() noexcept;

///
/// All the game state lifetimes.
///
static constexpr Lifetime kStateLifetimes[] =
{
    { InitGameWindow, CleanupGameWindow },
    { InitContext, CleanupContext },
    { InitGraphicsResources, CleanupGraphicsResources },
};

bool InitState() noexcept
{
    return TryInitItems(kStateLifetimes);
}

///
/// \param key The pressed key's virtual key code.
/// \param pressed Whether the key was pressed.
///
static void OnKeyEvent(int key, bool pressed) noexcept;

bool InitGameWindow() noexcept
{
    Window::SetKeyCallback(OnKeyEvent);
    sWindow = Window::MakeWindow(skWinSize, L"lepong");
    return sWindow;
}

///
/// What does this do?
///
static void OnKeyUp(int key) noexcept;

///
/// Huh.
///
static void OnKeyDown(int key) noexcept;

///
/// Sets the ball's direction to a random diagonal direction.
///
static void LaunchBall() noexcept;

void OnKeyEvent(int key, bool pressed) noexcept
{
    if (sPlaying)
    {
        if (pressed)
        {
            OnKeyDown(key);
        }
        else
        {
            OnKeyUp(key);
        }
    }
    else if (pressed && key == VK_SPACE)
    {
        sPlaying = true;
        LaunchBall();
    }
}

// Ugly input code below.

static constexpr int skP2Up = VK_UP;
static constexpr int skP2Down = VK_DOWN;
static constexpr int skP1Up = 'W';
static constexpr int skP1Down = 'S';

void OnKeyUp(int key) noexcept
{
    switch (key)
    {
    case skP2Up:
        sPaddle2.OnMoveUpReleased();
        break;

    case skP2Down:
        sPaddle2.OnMoveDownReleased();
        break;

    case skP1Up:
        sPaddle1.OnMoveUpReleased();
        break;

    case skP1Down:
        sPaddle1.OnMoveDownReleased();
        break;

    default: break;
    }
}

void OnKeyDown(int key) noexcept
{
    switch (key)
    {
    case skP2Up:
        sPaddle2.OnMoveUpPressed();
        break;

    case skP2Down:
        sPaddle2.OnMoveDownPressed();
        break;

    case skP1Up:
        sPaddle1.OnMoveUpPressed();
        break;

    case skP1Down:
        sPaddle1.OnMoveDownPressed();
        break;

    default: break;
    }
}

void LaunchBall() noexcept
{
    sBall.moveSpeed = Ball::skDefaultMoveSpeed;

    sBall.moveDirection = { RandomSignFloat(), RandomSignFloat() };
    sBall.moveDirection = Normalize(sBall.moveDirection);
}

void CleanupGameWindow() noexcept
{
    Window::DestroyWindow(sWindow);
}

bool InitContext() noexcept
{
    sContext = gl::MakeContext(sWindow);
    const auto kValid = sContext.IsValid();

    if (kValid)
    {
        gl::MakeContextCurrent(sContext);
    }

    return kValid;
}

void CleanupContext() noexcept
{
    gl::DestroyContext(sContext);
}

///
/// \return Can you guess?
///
LEPONG_NODISCARD static bool InitPaddleProgram() noexcept;

///
/// You guessed it.
///
static void CleanupPaddleProgram() noexcept;

///
/// I wonder what this does.
///
LEPONG_NODISCARD static bool InitBallProgram() noexcept;

///
/// Epic.
///
static void CleanupBallProgram() noexcept;

///
/// Oh boy.
///
LEPONG_NODISCARD static bool InitQuad() noexcept;

///
/// This is getting tedious.
///
static void CleanupQuad() noexcept;

///
/// Let's go.
///
LEPONG_NODISCARD static bool InitTexturedQuad() noexcept;

///
/// Where is my super suit?
///
static void CleanupTexturedQuad() noexcept;

///
/// All the graphics resource lifetimes.
///
static constexpr Lifetime skGraphicsResourceLifetimes[] =
{
    { InitPaddleProgram, CleanupPaddleProgram },
    { InitBallProgram, CleanupBallProgram },
    { InitQuad, CleanupQuad },
    { InitTexturedQuad, CleanupTexturedQuad },
};

bool InitGraphicsResources() noexcept
{
    return TryInitItems(skGraphicsResourceLifetimes);
}

///
/// Creates a program using the provided shaders and loads the <b>uWinSize</b> uniform.<br>
/// The provided shaders are then destroyed.
///
LEPONG_NODISCARD static GLuint CreateProgramWithWinSizeUniform(GLuint vertex, GLuint fragment) noexcept;

bool InitPaddleProgram() noexcept
{
    sPaddleProgram = CreateProgramWithWinSizeUniform(
        Graphics::MakeQuadVertexShader(), MakePaddleFragmentShader()
    );

    return sPaddleProgram;
}

///
/// Loads the window size value into the corresponding uniform in the provided program.
///
static void LoadWinSizeUniform(GLuint program) noexcept;

GLuint CreateProgramWithWinSizeUniform(GLuint vertex, GLuint fragment) noexcept
{
    const auto kProgram = Graphics::CreateProgramFromShaders(vertex, fragment);

    gl::DeleteShader(vertex);
    gl::DeleteShader(fragment);

    if (kProgram)
    {
        LoadWinSizeUniform(kProgram);
    }

    return kProgram;
}

void LoadWinSizeUniform(GLuint program) noexcept
{
    constexpr Vector2f kWinSize =
    {
        static_cast<float>(skWinSize.x),
        static_cast<float>(skWinSize.y)
    };

    gl::UseProgram(program);

    const auto kLocation = gl::GetUniformLocation(program, "uWinSize");
    gl::Uniform2f(kLocation, kWinSize.x, kWinSize.y);
}

void CleanupPaddleProgram() noexcept
{
    gl::DeleteProgram(sPaddleProgram);
}

bool InitBallProgram() noexcept
{
    sBallProgram = CreateProgramWithWinSizeUniform(
        Graphics::MakeTexturedQuadVertexShader(), MakeBallFragmentShader()
    );

    return sBallProgram;
}

void CleanupBallProgram() noexcept
{
    gl::DeleteProgram(sBallProgram);
}

bool InitQuad() noexcept
{
    sQuad = Graphics::MakeSimpleQuad();
    return sQuad.IsValid();
}

void CleanupQuad() noexcept
{
    Graphics::DestroyMesh(sQuad);
}

bool InitTexturedQuad() noexcept
{
    sTexturedQuad = Graphics::MakeTexturedQuad();
    return sTexturedQuad.IsValid();
}

void CleanupTexturedQuad() noexcept
{
    Graphics::DestroyMesh(sTexturedQuad);
}

void CleanupGraphicsResources() noexcept
{
    CleanupItems(skGraphicsResourceLifetimes);
}

void CleanupState() noexcept
{
    CleanupItems(kStateLifetimes);
}

static auto sRunning = false;

///
/// Called before entering the main loop.
///
static void OnBeginRun() noexcept;

///
/// Returns the time elapsed since this function was last called.
///
LEPONG_NODISCARD static float GetTimeDelta() noexcept;

///
/// Called at each game update.
///
static void OnUpdate(float delta) noexcept;

///
/// Called at each game frame.
///
static void OnRender() noexcept;

///
/// Called when exiting the main loop.
///
static void OnFinishRun() noexcept;

void Run() noexcept
{
    LEPONG_CHECK_OR_RETURN(sInitialized && !sRunning);

    sRunning = true;
    OnBeginRun();

    while (sRunning)
    {
        sRunning = Window::PollEvents();

        const auto cDelta = GetTimeDelta();
        OnUpdate(cDelta);

        OnRender();
    }

    OnFinishRun();
}

///
/// Logs the current context's specifications.
///
static void LogContextSpecifications() noexcept;

///
/// Resets the game state.
///
static void ResetGameState() noexcept;

///
/// Properly sets the paddle positions on the terrain.
///
static void PositionPaddlesOnTerrain() noexcept;

void OnBeginRun() noexcept
{
    Window::ShowWindow(sWindow);
    Window::SetWindowResizable(sWindow, false);

    LogContextSpecifications();
    ResetGameState();
    PositionPaddlesOnTerrain();

    const auto kCurrentTime = (unsigned)time(nullptr);
    srand(kCurrentTime);
}

#define LEPONG_LOG_GL_STRING(name) \
    Log::Log(reinterpret_cast<const char*>(gl::GetString(name)))

void LogContextSpecifications() noexcept
{
    LEPONG_LOG_GL_STRING(gl::Version);
    LEPONG_LOG_GL_STRING(gl::Vendor);
    LEPONG_LOG_GL_STRING(gl::Renderer);
}

void ResetGameState() noexcept
{
    sBall.Reset(skWinSize);

    sPaddle1.Reset(skWinSize);
    sPaddle2.Reset(skWinSize);

    sPlaying = false;
}

void PositionPaddlesOnTerrain() noexcept
{
    const auto kBorderOffset = 50.0f;

    sPaddle1.position.x = kBorderOffset;
    sPaddle2.position.x = skWinSize.x - kBorderOffset;
}

float GetTimeDelta() noexcept
{
    static auto sLastTime = 0.0f;

    const auto kNow = Time::Get();
    const auto kTimeDelta = kNow - sLastTime;
    sLastTime = kNow;

    return kTimeDelta;
}

///
/// Handles ball collision with terrain sides.
///
static void CheckBallSideCollision() noexcept;

void OnUpdate(float delta) noexcept
{
    sBall.Update(delta);

    sPaddle1.Update(delta, skWinSize);
    sPaddle2.Update(delta, skWinSize);

    sBall.CollideWithTerrain(skWinSize);

    const auto kCollides =
        sBall.CollideWith(sPaddle1) ||
        sBall.CollideWith(sPaddle2);

    if (!kCollides)
    {
        CheckBallSideCollision();
    }
}

///
/// Gives the point to the player corresponding to the provided side.<br>
/// The player who won the point is the player opposite to the provided side.
///
static void UpdateScores(Side lostSide) noexcept;

void CheckBallSideCollision() noexcept
{
    const auto kSide = sBall.GetTouchingSide(skWinSize);

    if (kSide != Side::None)
    {
        UpdateScores(kSide);
        ResetGameState();
    }
}

void UpdateScores(Side lostSide) noexcept
{
    const auto kScoreIndex = 1u - static_cast<unsigned>(lostSide);
    ++sPlayerScores[kScoreIndex];
}

void OnRender() noexcept
{
    gl::Clear(gl::ColorBufferBit);

    sBall.Render();

    sPaddle1.Render();
    sPaddle2.Render();

    gl::SwapBuffers(sContext);
}

void OnFinishRun() noexcept
{
    Window::HideWindow(sWindow);
}

void Cleanup() noexcept
{
    LEPONG_CHECK_OR_RETURN(sInitialized && !sRunning);

    CleanupItems(kGameLifetimes);
    sInitialized = false;
}

} // namespace lepong
