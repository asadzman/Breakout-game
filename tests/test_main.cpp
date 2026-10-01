#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "physics/Vec2.h"
#include "physics/Collision.h"
#include "raster/BitmapFont.h"
#include "core/Config.h"
#include "core/LevelManager.h"
#include "entities/Paddle.h"
#include "entities/Ball.h"
#include "entities/Brick.h"
#include "entities/PowerUp.h"

#include <QCoreApplication>

using namespace Breakout;

static int s_testsRun = 0;
static int s_testsPassed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        s_testsRun++; \
        std::cout << "[TEST] Running " << #fn << "... "; \
        if (fn()) { \
            s_testsPassed++; \
            std::cout << "PASSED\n"; \
        } \
    } while (0)

static bool testVec2Math() {
    Vec2 a(3.0f, 4.0f);
    TEST_ASSERT(std::abs(a.length() - 5.0f) < 1e-4f, "Vec2 length should be 5 for (3, 4)");
    TEST_ASSERT(std::abs(a.lengthSquared() - 25.0f) < 1e-4f, "Vec2 lengthSquared should be 25");

    Vec2 norm = a.normalized();
    TEST_ASSERT(std::abs(norm.length() - 1.0f) < 1e-4f, "Normalized vector length should be 1");
    TEST_ASSERT(std::abs(norm.x - 0.6f) < 1e-4f && std::abs(norm.y - 0.8f) < 1e-4f, "Normalized components mismatch");

    Vec2 b(1.0f, -2.0f);
    Vec2 c = a + b;
    TEST_ASSERT(c.x == 4.0f && c.y == 2.0f, "Vec2 addition failed");

    Vec2 normal(0.0f, -1.0f);
    Vec2 vel(10.0f, 10.0f);
    Vec2 ref = vel.reflected(normal);
    TEST_ASSERT(std::abs(ref.x - 10.0f) < 1e-4f, "Reflected X should be unchanged");
    TEST_ASSERT(std::abs(ref.y - (-10.0f)) < 1e-4f, "Reflected Y should be inverted");

    return true;
}

static bool testCollisionCircleAABB() {
    AABB box(50.0f, 50.0f, 20.0f, 10.0f); // [50, 70] x [50, 60]

    // 1. Outside - no collision
    {
        CollisionResult r = Collision::testCircleAABB(Vec2(20.0f, 20.0f), 3.0f, box);
        TEST_ASSERT(!r.collided, "Should not collide when distant");
    }

    // 2. Collision with top edge (moving down)
    {
        CollisionResult r = Collision::testCircleAABB(Vec2(60.0f, 48.0f), 3.0f, box);
        TEST_ASSERT(r.collided, "Should collide with top edge");
        TEST_ASSERT(r.normal.y < -0.9f, "Top collision normal should point up");
        TEST_ASSERT(r.penetration > 0.5f, "Penetration depth should be positive");
    }

    // 3. Collision with bottom edge
    {
        CollisionResult r = Collision::testCircleAABB(Vec2(60.0f, 62.0f), 3.0f, box);
        TEST_ASSERT(r.collided, "Should collide with bottom edge");
        TEST_ASSERT(r.normal.y > 0.9f, "Bottom collision normal should point down");
    }

    // 4. Collision with left edge
    {
        CollisionResult r = Collision::testCircleAABB(Vec2(48.0f, 55.0f), 3.0f, box);
        TEST_ASSERT(r.collided, "Should collide with left edge");
        TEST_ASSERT(r.normal.x < -0.9f, "Left collision normal should point left");
    }

    // 5. Center inside box
    {
        CollisionResult r = Collision::testCircleAABB(Vec2(60.0f, 52.0f), 3.0f, box);
        TEST_ASSERT(r.collided, "Should collide when inside box");
        TEST_ASSERT(r.normal.y < 0.0f, "Should eject along minimum penetration axis");
    }

    return true;
}

static bool testAABBAABBCollision() {
    AABB a(10.0f, 10.0f, 20.0f, 20.0f);
    AABB b(25.0f, 25.0f, 20.0f, 20.0f);
    AABB c(60.0f, 60.0f, 10.0f, 10.0f);

    TEST_ASSERT(Collision::testAABBAABB(a, b), "Boxes a and b should intersect");
    TEST_ASSERT(!Collision::testAABBAABB(a, c), "Boxes a and c should not intersect");

    return true;
}

static bool testBitmapFont() {
    int w1 = BitmapFont::measureText("A", 1);
    TEST_ASSERT(w1 == BitmapFont::GLYPH_WIDTH, "Single character width should equal GLYPH_WIDTH");

    int w2 = BitmapFont::measureText("HELLO", 1, 1);
    TEST_ASSERT(w2 == 5 * 8 + 4 * 1, "HELLO width calculation incorrect");

    const uint8_t* glyphA = BitmapFont::getGlyph('A');
    TEST_ASSERT(glyphA != nullptr, "Glyph for 'A' should not be null");

    // Non-ASCII fallback check
    const uint8_t* glyphInvalid = BitmapFont::getGlyph(static_cast<char>(200));
    TEST_ASSERT(glyphInvalid != nullptr, "Fallback glyph should not be null");

    return true;
}

static bool testPaddleControls() {
    Paddle paddle;
    paddle.reset();
    float startX = paddle.getPosition().x;

    // Move right via keyboard
    paddle.setMoveRight(true);
    paddle.update(0.1f);
    TEST_ASSERT(paddle.getPosition().x > startX, "Paddle should move right with keyboard input");

    // Move with mouse target
    paddle.setTargetX(100.0f);
    paddle.update(0.1f);
    TEST_ASSERT(std::abs(paddle.getPosition().x - (100.0f - paddle.getWidth() * 0.5f)) < 1.0f, "Paddle should glide to mouse target");

    // Re-assert keyboard: releasing Right and pressing Left arrow should reclaim priority from mouse
    paddle.setMoveRight(false);
    paddle.setMoveLeft(true);
    float mouseX = paddle.getPosition().x;
    paddle.update(0.1f);
    TEST_ASSERT(paddle.getPosition().x < mouseX, "Keyboard should immediately reclaim priority over mouse control");

    return true;
}

static bool testBallPhysicsDeflection() {
    Ball ball(Vec2(100.0f, 200.0f), Vec2(0.0f, 160.0f));

    // Deflect off paddle center
    ball.deflectPaddle(100.0f, 44.0f, 0.0f);
    TEST_ASSERT(ball.getVelocity().y < -45.0f, "Deflected ball must travel upward with |vy| >= 45");

    // Deflect off left edge of paddle
    ball.setPosition(Vec2(80.0f, 200.0f));
    ball.setVelocity(Vec2(0.0f, 160.0f));
    ball.deflectPaddle(100.0f, 44.0f, -50.0f);
    TEST_ASSERT(ball.getVelocity().x < 0.0f, "Left edge hit should produce negative vx");
    TEST_ASSERT(ball.getVelocity().y < -45.0f, "Must maintain upward minimum velocity");

    return true;
}

static bool testLevelManager() {
    LevelManager lm;
    lm.discoverLevels("assets/levels");
    TEST_ASSERT(lm.getTotalLevels() >= 6, "Should discover at least 6 stages");

    LevelData l1;
    bool ok = lm.peekLevel(0, l1);
    TEST_ASSERT(ok, "Should peek level 1 successfully");
    TEST_ASSERT(!l1.bricks.empty(), "Level 1 must contain bricks");
    TEST_ASSERT(l1.destructibleCount > 0, "Level 1 must have destructible bricks");

    return true;
}

static bool testBrickHits() {
    Brick brick(10.0f, 10.0f, 28.0f, 9.0f, BrickType::Armored, 2, Colors::NeonYellow);
    TEST_ASSERT(!brick.isDestroyed(), "New brick should not be destroyed");

    bool destroyed1 = brick.hit(1);
    TEST_ASSERT(!destroyed1, "Armored brick with 2 HP should survive first hit");
    TEST_ASSERT(brick.getHp() == 1, "Armored brick should have 1 HP remaining");

    bool destroyed2 = brick.hit(1);
    TEST_ASSERT(destroyed2, "Armored brick should be destroyed after 2nd hit");
    TEST_ASSERT(brick.isDestroyed(), "Brick isDestroyed() should be true");

    return true;
}

static bool testPowerUpDimensions() {
    TEST_ASSERT(POWERUP_WIDTH >= 16.0f, "POWERUP_WIDTH must be at least 16 for 8x8 font");
    TEST_ASSERT(POWERUP_HEIGHT >= 10.0f, "POWERUP_HEIGHT must be at least 10 for 8x8 font");

    PowerUp pup(Vec2(100.0f, 100.0f), PowerUpType::Fireball);
    TEST_ASSERT(pup.getIcon(PowerUpType::Fireball) == 'F', "Fireball icon should be F");
    TEST_ASSERT(pup.getIcon(PowerUpType::SlowBall) == 'Z', "SlowBall icon should be Z");
    TEST_ASSERT(pup.getIcon(PowerUpType::Shield) == 'B', "Shield icon should be B");

    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "========================================\n";
    std::cout << "  Retro Breakout - Cross-Platform Tests\n";
    std::cout << "========================================\n";

    RUN_TEST(testVec2Math);
    RUN_TEST(testCollisionCircleAABB);
    RUN_TEST(testAABBAABBCollision);
    RUN_TEST(testBitmapFont);
    RUN_TEST(testPaddleControls);
    RUN_TEST(testBallPhysicsDeflection);
    RUN_TEST(testLevelManager);
    RUN_TEST(testBrickHits);
    RUN_TEST(testPowerUpDimensions);

    std::cout << "----------------------------------------\n";
    std::cout << "Results: " << s_testsPassed << " / " << s_testsRun << " tests passed.\n";

    if (s_testsPassed == s_testsRun) {
        std::cout << "ALL TESTS PASSED SUCCESSFULLY!\n";
        return 0;
    } else {
        std::cerr << "SOME TESTS FAILED!\n";
        return 1;
    }
}
