#include <iostream>
#include <vector>
#include <chrono>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;

// This is a modern C++11 'typedef' replacement:
// we just defined an alias for 'float', called 'FrameTime'
using FrameTime = float;

// using Integer = int; -> typedef int Integer;

// Using templates:
//  template<typename T> using MyVector = Vector2<T>
//  MyVector<float> == Vector2<float>

// 'constexpr' defines an immutable compile-time value
constexpr int windowWidth{800}, windowHeight{600}; // {} - uniform initialization
constexpr float ballRadius{10.0f}, ballVelocity{0.4f};

// Let's create some constants for the paddle
constexpr float paddleWidth{60.f}, paddleHeight{20.f}, paddleVelocity{1.f};

// Let's define some constants for the bricks
constexpr float blockWidth{60.f}, blockHeight{20.f};
constexpr int countBlocksX{11}, countBlocksY{4};

// Let's define a step and a slice constant
constexpr float ftStep{1.f}, ftSlice{1.f};

// Ball class
// struct == 'class' in C++ but everything is public
struct Ball {
    // CircleShape is an SFML class that
    // defines a renderable circle
    CircleShape shape;

    // 2D vector that stores the Ball's velocity
    Vector2f velocity{-ballVelocity, -ballVelocity};

    // create the ball constructor
    Ball(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setRadius(ballRadius);
        shape.setFillColor(Color::Red);
        shape.setOrigin(ballRadius, ballRadius);
    }

    void resetBall()
    {
        shape.setPosition(windowWidth / 2, windowHeight / 2);
        velocity.y = -ballVelocity;
    }

    // 'update' the ball: move its shape
    // by the current velocity
    bool update(FrameTime mFT) {
        shape.move(velocity * mFT);

        // prevent the ball to get outside the window
        if(bottom() > windowHeight) velocity.y = velocity.y;
            //resetBall();

        if(left() < 0 || right() > windowWidth) velocity.x = -velocity.x;
        if(top() < 0) velocity.y = -velocity.y;

        return true;
    }

       // Create 'property' methods to easily
    // get common used values

    // A very important thing in C++ is const-correcctness:
    // since 'getter' functions are not modifying any member
    // of their callers, they should be marked as const - this
    // allows to call them as 'const&' ans 'const*' parents

    // And we should also mark the functions as 'noexcept',
    // a new C++ keyword that allows the programmer to help
    // the compiler optimize the code by marking certain functions
    // that will never throw an exception

    float x() const noexcept { return shape.getPosition().x; }
    float y() const noexcept { return shape.getPosition().y; }
    float left() const noexcept { return x() - shape.getRadius(); }
    float right() const noexcept { return x() + shape.getRadius(); }
    float top() const noexcept { return y() - shape.getRadius(); }
    float bottom() const noexcept { return y() + shape.getRadius(); }
};

// We can now refactor our code by creating a Rectangle class
// that encapsulates the common properties for 'Brick' and 'Paddle'.
// There is no run-time overhead when we avoid using 'virtual' methods
// in hierarchies
struct Rectangle
{
    // RectangleShape is an SFML class that defines
    // a renderable rectangular shape
    RectangleShape shape;

    // Create 'property' methods to easily
    // get common used values
    float x() const noexcept { return shape.getPosition().x; }
    float y() const noexcept { return shape.getPosition().y; }
    float left() const noexcept { return x() - shape.getSize().x / 2.f; }
    float right() const noexcept { return x() + shape.getSize().x / 2.f; }
    float top() const noexcept { return y() - shape.getSize().y / 2.f; }
    float bottom() const noexcept { return y() + shape.getSize().y / 2.f; }
};

struct Paddle : public Rectangle
{
    Vector2f velocity;

    Paddle(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({paddleWidth, paddleHeight}); //shape.setSize(Vector2f(paddleWidth, paddleHeight));
        shape.setFillColor(Color::Blue);
        shape.setOrigin(paddleWidth / 2.f, paddleHeight / 2.f);
    }

    void update(FrameTime mFT)
    {
        shape.move(velocity * mFT);

        // keep the paddle inside the window
        if(Keyboard::isKeyPressed(Keyboard::Key::Left) && left() > 0)
            velocity.x = -paddleVelocity;
        else if(Keyboard::isKeyPressed(Keyboard::Key::Right) && right() < windowWidth)
            velocity.x = paddleVelocity;
        else
            velocity.x = 0;
    }
};

// Brick class
struct Brick : public Rectangle
{
    //boolean value to check wether the brick has been hit
    bool destroyed{false};

    Brick(float mX, float mY)
    {
        shape.setPosition(mX, mY);
        shape.setSize({blockWidth, blockHeight});
        shape.setFillColor(Color::Yellow);
        shape.setOrigin(blockWidth / 2.f, blockHeight / 2.f);
    }

};

// Dealing with collisions: let's define a generix function
// to check if two shapes are intersecting (colliding)
template<class T1, class T2>
bool isIntersecting(T1& mA, T2& mB)
{
    return mA.right() >= mB.left() && mA.left() <= mB.right() &&
          mA.bottom() >= mB.top() && mA.top() <= mB.bottom();
};

void testCollision(Paddle& mPaddle, Ball& mBall)
{
    if(!isIntersecting(mPaddle, mBall)) return;

    // push the ball upwards
    mBall.velocity.y = -ballVelocity;

    // And let's direct it dependently on the position where the paddle was hit
    if(mBall.x() < mPaddle.x()) mBall.velocity.x = -ballVelocity;
    else mBall.velocity.x = ballVelocity;
}

void testCollision(Brick& mBrick, Ball& mBall)
{
    if (!isIntersecting(mBrick, mBall)) return;

    mBrick.destroyed = true;

    // Let's calculate how much of the ball intersects the brick
    // in every direction
    float overlapLeft{mBall.right() - mBrick.left()};
    float overlapRight{mBrick.right() - mBall.left()};
    float overlapTop{mBall.bottom() - mBrick.top()};
    float overlapBottom{mBrick.bottom() - mBall.top()};

    bool ballFromLeft(overlapLeft < overlapRight);
    bool ballFromTop(overlapTop < overlapBottom);

    // Let's store the minimum overlaps for the X and the Y axes
    float minOverlapX{ballFromLeft ? overlapLeft : overlapRight};
    float minOverlapY{ballFromTop ? overlapTop : overlapBottom};

    if(minOverlapX < minOverlapY)
        mBall.velocity.x = ballFromLeft ? -ballVelocity : ballVelocity;
    else
        mBall.velocity.y = ballFromTop ? -ballVelocity  : ballVelocity;
}

struct Game {
    // These menbers are related to the control of the game
    RenderWindow window{{windowWidth, windowHeight}, "Arkanoid"};
    FrameTime lastFT{0.f}, currentSlice{0.f};
    bool running{false};

    // These members are game entities
    Ball ball{windowWidth / 2, windowHeight / 2};
    Paddle paddle{windowWidth / 2, windowHeight - 50};
    vector<Brick>   bricks;

    Game()
    {
        // On construction we initalize the window and create
        // the brick wall.
        window.setFramerateLimit(240);

        // Fill in the vector, creating bricks in a grid-like pattern
        for(int iX{0}; iX < countBlocksX; ++iX)
            for(int iY{0}; iY < countBlocksY; ++iY) {
                bricks.emplace_back((iX + 1) * (blockWidth + 3) + 22,
                                    (iY + 2) * (blockHeight + 3));
            }
    }

    void inputPhase()
    {
        Event event;
        while(window.pollEvent(event)) {
            if(event.type == Event::Closed){
                window.close();
            }
        }

        if(Keyboard::isKeyPressed(Keyboard::Key::Escape)) running = false;
        if(Keyboard::isKeyPressed(Keyboard::Key::Space)) ball.resetBall();
    }

    void updatePhase()
    {
        currentSlice += lastFT;

        for (; currentSlice >= ftSlice ; currentSlice -= ftSlice) {
            ball.update(ftStep);
            paddle.update(ftStep);

            testCollision(paddle, ball);

            // test collision between ball and EVERY brick
            for (auto &brick : bricks) testCollision(brick, ball);

            // use an STL algorithm to remove all the destroyed blocks
            // from the block vector - using a cool C++11 lambda!!!!
            bricks.erase(remove_if(begin(bricks), end(bricks),
                                   [](const Brick &mBrick) { return mBrick.destroyed; }), end(bricks));
        }
    }

    void drawPhase()
    {
        window.draw(ball.shape);
        window.draw(paddle.shape);

        // We must draw every brick on the window
        // Let's use a C++11 foreach loop (draw every 'brick' in bricks
        // for(int i{0}; i < bricks.size(); ++i) window.draw(bricks[i].shape);
        for (auto &brick : bricks) window.draw(brick.shape);

        window.display();
    }

    void run()
    {
        // The 'run()' method is used to start the game and contains the game loop

        // Instead of using 'break' to stop the game, we will use a boolean - 'running'
        running = true;
        while(running) {
            auto timePoint1{chrono::high_resolution_clock::now()};

            window.clear(Color::Black);

            inputPhase();
            updatePhase();
            drawPhase();

            // End of our time interval
            auto timePoint2{chrono::high_resolution_clock::now()};

            auto elapsedTime(timePoint2 - timePoint1);

            FrameTime ft{chrono::duration_cast<chrono::duration<float, milli>>(elapsedTime).count()};
            lastFT = ft;

            auto ftSeconds(ft / 1000.f);
            auto fps(1.f / ftSeconds);

            window.setTitle("FT: " + to_string(ft) + "\tFPS: " + to_string(fps));

        }
    }
};

bool isRunning = false;

int main()
{
    // Let's create a temporary variable of type 'Game' and
    // run it, to start the game
    Game{}.run();

    return 0;
}