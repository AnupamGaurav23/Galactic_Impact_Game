//project by Anupam Gaurav


#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

#include "olcConsoleGameEngine.h"

class Anupam_Comets : public olcConsoleGameEngine
{
public:
    Anupam_Comets()
    {
        m_sAppName = L"Comets";
    }

private:
    struct sSpaceObject
    {
        int nSize;
        float x;
        float y;
        float dx;
        float dy;
        float angle;
    };

    vector<sSpaceObject> vecComets;
    vector<sSpaceObject> vecBullets;
    sSpaceObject player;
    bool bDead = false;
    int nScore = 0;

    vector<pair<float, float>> vecModelUFO;
    vector<pair<float, float>> vecModelComet;

protected:
    // Called by olcConsoleGameEngine
    virtual bool OnUserCreate()
    {
        vecModelUFO = 
        {
            { 0.0f, -5.0f},
            {-2.5f, +2.5f},
            {+2.5f, +2.5f}
        }; // A simple Isoceles Triangle

        // Create a "jagged" circle for the Comet. It's important it remains
        // mostly circular, as we do a simple collision check against a perfect
        // circle.
        int verts = 20;
        for (int i = 0; i < verts; i++)
        {
            float noise = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
            vecModelComet.push_back(make_pair(noise * sinf(((float)i / (float)verts) * 6.28318f), 
                                                 noise * cosf(((float)i / (float)verts) * 6.28318f)));
        }

        ResetGame();
        return true;
    }

    void ResetGame()
    {
        // Initialise Player Position
        player.x = ScreenWidth() / 2.0f;
        player.y = ScreenHeight() / 2.0f;
        player.dx = 0.0f;
        player.dy = 0.0f;
        player.angle = 0.0f;

        vecBullets.clear();
        vecComets.clear();

        // Put in two Comets
        vecComets.push_back({ (int)16, 20.0f, 20.0f, 8.0f, -6.0f, 0.0f });
        vecComets.push_back({ (int)16, 100.0f, 20.0f, -5.0f, 3.0f, 0.0f });

        // Reset game
        bDead = false;
        nScore = false;
    }

    // Implements "wrap around" for various in-game sytems
    void WrapCoordinates(float ix, float iy, float &ox, float &oy)
    {
        ox = ix;
        oy = iy;
        if (ix < 0.0f)  ox = ix + (float)ScreenWidth();
        if (ix >= (float)ScreenWidth()) ox = ix - (float)ScreenWidth();
        if (iy < 0.0f)  oy = iy + (float)ScreenHeight();
        if (iy >= (float)ScreenHeight()) oy = iy - (float)ScreenHeight();
    }

    // Overriden to handle toroidal drawing routines
    virtual void Draw(int x, int y, wchar_t c = 0x2588, short col = 0x000F)
    {
        float fx, fy;
        WrapCoordinates(x, y, fx, fy);      
        olcConsoleGameEngine::Draw(fx, fy, c, col);
    }

    bool IsPointInsideCircle(float cx, float cy, float radius, float x, float y)
    {
        return sqrt((x-cx)*(x-cx) + (y-cy)*(y-cy)) < radius;
    }

    // Called by olcConsoleGameEngine
    virtual bool OnUserUpdate(float fElapsedTime)
    {
        if (bDead)
            ResetGame();

        // Clear Screen
        Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, 0);

        // Steer UFO
        if (m_keys[VK_LEFT].bHeld)
            player.angle -= 5.0f * fElapsedTime;
        if (m_keys[VK_RIGHT].bHeld)
            player.angle += 5.0f * fElapsedTime;

        // Thrust? Apply ACCELERATION
        if (m_keys[VK_UP].bHeld)
        {
            // ACCELERATION changes VELOCITY (with respect to time)
            player.dx += sin(player.angle) * 20.0f * fElapsedTime;
            player.dy += -cos(player.angle) * 20.0f * fElapsedTime;
        }

        // VELOCITY changes POSITION (with respect to time)
        player.x += player.dx * fElapsedTime;
        player.y += player.dy * fElapsedTime;

        // Keep UFO in gamespace
        WrapCoordinates(player.x, player.y, player.x, player.y);

        // Check UFO collision with Comets
        for (auto &a : vecComets)
            if (IsPointInsideCircle(a.x, a.y, a.nSize, player.x, player.y))
                bDead = true; // Uh oh...

        // Fire Bullet in direction of player
        if (m_keys[VK_SPACE].bReleased)
            vecBullets.push_back({ 0, player.x, player.y, 50.0f * sinf(player.angle), -50.0f * cosf(player.angle), 100.0f });

        // Update and draw Comets
        for (auto &a : vecComets)
        {
            // VELOCITY changes POSITION (with respect to time)
            a.x += a.dx * fElapsedTime;
            a.y += a.dy * fElapsedTime;
            a.angle += 0.5f * fElapsedTime; // Add swanky rotation :)

            // Comet coordinates are kept in game space (toroidal mapping)
            WrapCoordinates(a.x, a.y, a.x, a.y);

            // Draw Comets
            DrawWireFrameModel(vecModelComet, a.x, a.y, a.angle, (float)a.nSize, FG_YELLOW); 
        }

        // Any new Comets created after collision detection are stored
        // in a temporary vector, so we don't interfere with the Comets
        // vector iterator in the for(auto)
        vector<sSpaceObject> newComets;

        // Update Bullets
        for (auto &b : vecBullets)
        {
            b.x += b.dx * fElapsedTime;
            b.y += b.dy * fElapsedTime;
            WrapCoordinates(b.x, b.y, b.x, b.y);
            b.angle -= 1.0f * fElapsedTime;

            // Check collision with Comets
            for (auto &a : vecComets)
            {
                //if (IsPointInsideRectangle(a.x, a.y, a.x + a.nSize, a.y + a.nSize, b.x, b.y))
                if(IsPointInsideCircle(a.x, a.y, a.nSize, b.x, b.y))
                {
                    // Comet Hit - Remove bullet
                    // We've already updated the bullets, so force bullet to be offscreen
                    // so it is cleaned up by the removal algorithm. 
                    b.x = -100;

                    // Create child Comets
                    if (a.nSize > 4)
                    {
                        float angle1 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
                        float angle2 = ((float)rand() / (float)RAND_MAX) * 6.283185f;
                        newComets.push_back({ (int)a.nSize >> 1 ,a.x, a.y, 10.0f * sinf(angle1), 10.0f * cosf(angle1), 0.0f });
                        newComets.push_back({ (int)a.nSize >> 1 ,a.x, a.y, 10.0f * sinf(angle2), 10.0f * cosf(angle2), 0.0f });
                    }

                    // Remove Comet - Same approach as bullets
                    a.x = -100;
                    nScore += 100; // Small score increase for hitting Comet
                }
            }
        }

        // Append new Comets to existing vector
        for(auto a:newComets)
            vecComets.push_back(a);

        // Clear up dead objects - They are out of game space

        // Remove Comets that have been blown up
        if (vecComets.size() > 0)
        {
            auto i = remove_if(vecComets.begin(), vecComets.end(), [&](sSpaceObject o) { return (o.x < 0); });
            if (i != vecComets.end())
                vecComets.erase(i);
        }

        if (vecComets.empty()) // If no Comets, level complete! :) - you win MORE Comets!
        {
            // Level Clear
            nScore += 1000; // Large score for level progression
            vecComets.clear();
            vecBullets.clear();

            // Add two new Comets, but in a place where the player is not, we'll simply
            // add them 90 degrees left and right to the player, their coordinates will
            // be wrapped by th enext Comet update
            vecComets.push_back({ (int)16, 30.0f * sinf(player.angle - 3.14159f/2.0f) + player.x,
                                              30.0f * cosf(player.angle - 3.14159f/2.0f) + player.y,
                                              10.0f * sinf(player.angle), 10.0f*cosf(player.angle), 0.0f });

            vecComets.push_back({ (int)16, 30.0f * sinf(player.angle + 3.14159f/2.0f) + player.x,
                                              30.0f * cosf(player.angle + 3.14159f/2.0f) + player.y,
                                              10.0f * sinf(-player.angle), 10.0f*cosf(-player.angle), 0.0f });
        }

        // Remove bullets that have gone off screen
        if (vecBullets.size() > 0)
        {
            auto i = remove_if(vecBullets.begin(), vecBullets.end(), [&](sSpaceObject o) { return (o.x < 1 || o.y < 1 || o.x >= ScreenWidth() - 1 || o.y >= ScreenHeight() - 1); });
            if (i != vecBullets.end())
                vecBullets.erase(i);
        }

        // Draw Bullets
        for (auto b : vecBullets)
            Draw(b.x, b.y);

        // Draw UFO
        DrawWireFrameModel(vecModelUFO, player.x, player.y, player.angle);

        // Draw Score
        DrawString(2, 2, L"SCORE: " + to_wstring(nScore));
        return true;
    }

    void DrawWireFrameModel(const vector<pair<float, float>> &vecModelCoordinates, float x, float y, float r = 0.0f, float s = 1.0f, short col = FG_WHITE)
    {
        // pair.first = x coordinate
        // pair.second = y coordinate
        
        // Create translated model vector of coordinate pairs
        vector<pair<float, float>> vecTransformedCoordinates;
        int verts = vecModelCoordinates.size();
        vecTransformedCoordinates.resize(verts);

        // Rotate
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecModelCoordinates[i].first * cosf(r) - vecModelCoordinates[i].second * sinf(r);
            vecTransformedCoordinates[i].second = vecModelCoordinates[i].first * sinf(r) + vecModelCoordinates[i].second * cosf(r);
        }

        // Scale
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * s;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * s;
        }

        // Translate
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
        }

        // Draw Closed Polygon
        for (int i = 0; i < verts + 1; i++)
        {
            int j = (i + 1);
            DrawLine(vecTransformedCoordinates[i % verts].first, vecTransformedCoordinates[i % verts].second, 
                vecTransformedCoordinates[j % verts].first, vecTransformedCoordinates[j % verts].second, PIXEL_SOLID, col);
        }
    }
};


int main()
{
    // Use olcConsoleGameEngine derived app
    Anupam_Comets game;
    game.ConstructConsole(160, 100, 8, 8);
    game.Start();
    return 0;
}
