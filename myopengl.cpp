// space_shooter_commented.c
#ifdef _WIN32
#include <windows.h>     // Required for Windows builds (windowing and types)
#endif

#include <stdio.h>        // printf, snprintf
#include <stdlib.h>       // exit
#include <string.h>       // string ops (not heavily used here)
#include <GL/glut.h>      // OpenGL Utility Toolkit (GLUT)
#include <math.h>         // math functions like roundf

#define GL_SILENCE_DEPRECATION // suppress deprecation warnings on some platforms

// -----------------------------------------------------------------------------
// Constants and global state
// -----------------------------------------------------------------------------

// World extents (these match the orthographic projection used in init())
#define XMAX 1200
#define YMAX 700

// Movement speed in world units per second (use delta time to make framerate independent)
#define SPACESHIP_SPEED 600.0f

// How long (in seconds) a fired laser remains visible
#define LASER_DURATION_SECONDS 0.6f

// Cached OpenGL viewport (x, y, width, height) used to convert mouse pixels -> world coords
GLint m_viewport[4];

// Mouse/GUI state (left-button pressed? world-space mouse coordinates)
bool mButtonPressed = false;
float mouseX = 0.0f, mouseY = 0.0f;

// Game view states (menu flow)
enum view { INTRO, MENU, INSTRUCTIONS, GAME, GAMEOVER };
view viewPage = INTRO; // start at intro screen

// Keyboard state array: keyStates[c] == true while key 'c' is pressed
bool keyStates[256] = { false };

// Per-ship aiming flags during firing: [0] = aim up, [1] = aim down
bool laserLeftDir[2] = { false };   // left player's vertical aim while firing
bool laserRightDir[2] = { false };  // right player's vertical aim while firing

// Hit points for each ship
int lifeLeft = 100;
int lifeRight = 100;

// Ship positions; ships are positioned symmetrically: left ship is drawn at -xLeft
float xLeft = 500.0f, yLeft = 0.0f;
float xRight = 500.0f, yRight = 0.0f;

// Laser visibility state and timers (when true the beam is visible until the timer runs out)
bool laserLeft = false, laserRight = false;
float laserLeftTimer = 0.0f, laserRightTimer = 0.0f;

// One-shot gating: require releasing the fire key before you can shoot again
bool canShootLeft = true;   // controls left player 'c'
bool canShootRight = true;  // controls right player 'm'

// Per-beam "already applied damage" flags. Prevents a single beam from reducing life repeatedly.
bool laserLeftHasHit = false;
bool laserRightHasHit = false;

// Index used to animate the spaceship lights (rotating color index)
GLint CI = 0;

// Time tracking for frame-independent movement: lastTimeMs & deltaTime (seconds)
int lastTimeMs = 0;
float deltaTime = 0.0f; // seconds elapsed since previous frame

// ------------------- NEW FEATURE: Scoreboard (simple) -------------------
// Score awarded to the shooter when they land a hit (easy feature).
int scoreLeft = 0;
int scoreRight = 0;
// -----------------------------------------------------------------------

// Visual geometry arrays (vertices for alien parts & light color table)
GLfloat LightColor[][3] = { {1,1,0}, {0,1,1}, {0,1,0} };

GLfloat AlienBody[][2] = {
    {-4,9}, {-6,0}, {0,0}, {0.5,9}, {0.15,12}, {-14,18}, {-19,10}, {-20,0}, {-6,0}
};

GLfloat AlienCollar[][2] = {
    {-9,10.5}, {-6,11}, {-5,12}, {6,18}, {10,20}, {13,23}, {16,30}, {19,39}, {16,38},
    {10,37}, {-13,39}, {-18,41}, {-20,43}, {-20.5,42}, {-21,30}, {-19.5,23}, {-19,20},
    {-14,16}, {-15,17}, {-13,13},  {-9,10.5}
};

GLfloat ALienFace[][2] = {
    {-6,11}, {-4.5,18}, {0.5,20}, {0.,20.5}, {0.1,19.5}, {1.8,19}, {5,20}, {7,23}, {9,29},
    {6,29.5}, {5,28}, {7,30}, {10,38}, {11,38}, {11,40}, {11.5,48}, {10,50.5}, {8.5,51}, {6,52},
    {1,51}, {-3,50}, {-1,51}, {-3,52}, {-5,52.5}, {-6,52}, {-9,51}, {-10.5,50}, {-12,49}, {-12.5,47},
    {-12,43}, {-13,40}, {-12,38.5}, {-13.5,33}, {-15,38}, {-14.5,32}, {-14,28}, {-13.5,33}, {-14,28},
    {-13.8,24}, {-13,20}, {-11,19}, {-10.5,12}, {-6,11}
};

GLfloat ALienBeak[][2] = {
    {-6,21.5}, {-6.5,22}, {-9,21}, {-11,20.5}, {-20,20}, {-14,23}, {-9.5,28}, {-7,27}, {-6,26.5},
    {-4.5,23}, {-4,21}, {-6,19.5}, {-8.5,19}, {-10,19.5}, {-11,20.5}
};

// -----------------------------------------------------------------------------
// Helper utilities
// -----------------------------------------------------------------------------

// Draw bitmap text at world coordinates (x,y,z) using GLUT bitmap font.
// Uses glRasterPos3f internally to place the text.
void displayRasterText(float x, float y, float z, const char* stringToDisplay) {
    glRasterPos3f(x, y, z);
    for (const char* c = stringToDisplay; *c != '\0'; c++) {
        // Draw each character in the string using GLUT's TIMES_ROMAN_24 font
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }
}

// Initialize OpenGL state and projection matrix
void init()
{
    glClearColor(0, 0, 0, 0);            // black background
    glMatrixMode(GL_PROJECTION);         // switch to projection matrix
    glLoadIdentity();                    // reset projection
    // Set an orthographic projection that matches our XMAX/YMAX constants
    gluOrtho2D(-1200, 1200, -700, 700);
    glMatrixMode(GL_MODELVIEW);          // return to modelview

    lastTimeMs = glutGet(GLUT_ELAPSED_TIME); // initialize timing baseline
}

// -----------------------------------------------------------------------------
// Screen renderers (render content for specific screens). These do not call
// glutSwapBuffers() — caller display() handles the single swap per frame.
// -----------------------------------------------------------------------------

// Intro screen rendering
void introScreen()
{
    glClear(GL_COLOR_BUFFER_BIT); // clear color buffer for fresh frame
    glColor3f(1, 0, 0);           // red text
    displayRasterText(-425, 490, 0.0f, "NMAM INSTITUTE OF TECHNOLOGY");
    glColor3f(1, 1, 1);           // white text
    displayRasterText(-700, 385, 0.0f, "DEPARTMENT OF COMPUTER SCIENCE AND ENGINEERING");
    displayRasterText(-300, -550, 0.0f, "Press ENTER to start the game");
    // NOTE: display() will swap buffers once per frame, so do not swap here
}

// Menu / start screen rendering
void startScreenDisplay()
{
    // We don't clear here — display() already cleared the frame.
    glLineWidth(10);             // thick outline for menu border
    glColor3f(1, 0, 0);          // red border color
    glBegin(GL_LINE_LOOP);
    glVertex2f(-750, -500); glVertex2f(-750, 550); glVertex2f(750, 550); glVertex2f(750, -500);
    glEnd();
    glLineWidth(1);              // reset line width

    // Draw Start / Instructions / Quit buttons as filled rectangles
    glColor3f(1, 1, 0); // yellow buttons
    glBegin(GL_POLYGON); glVertex2f(-200, 300); glVertex2f(-200, 400); glVertex2f(200, 400); glVertex2f(200, 300); glEnd();
    glBegin(GL_POLYGON); glVertex2f(-200, 50);  glVertex2f(-200, 150); glVertex2f(200, 150); glVertex2f(200, 50);  glEnd();
    glBegin(GL_POLYGON); glVertex2f(-200, -200);glVertex2f(-200, -100);glVertex2f(200, -100);glVertex2f(200, -200);glEnd();

    // Highlight and click handling for Start button using mouse world coords (mouseX, mouseY)
    if (mouseX >= -100 && mouseX <= 100 && mouseY >= 150 && mouseY <= 200) {
        glColor3f(0, 0, 1); // hover color (blue)
        if (mButtonPressed) {
            // start game when user clicks start
            lifeLeft = lifeRight = 100;     // reset life
            scoreLeft = scoreRight = 0;     // reset scoreboard
            viewPage = GAME;                // switch to game view
            mButtonPressed = false;         // consume click
        }
    }
    else {
        glColor3f(0, 0, 0); // regular text color when not hovered
    }
    displayRasterText(-100, 340, 0.4f, "Start Game"); // label for start

    // Instructions button hover and click
    if (mouseX >= -100 && mouseX <= 100 && mouseY >= 30 && mouseY <= 80) {
        glColor3f(0, 0, 1);
        if (mButtonPressed) { viewPage = INSTRUCTIONS; mButtonPressed = false; }
    }
    else glColor3f(0, 0, 0);
    displayRasterText(-120, 80, 0.4f, "Instructions");

    // Quit button hover and click
    if (mouseX >= -100 && mouseX <= 100 && mouseY >= -90 && mouseY <= -40) {
        glColor3f(0, 0, 1);
        if (mButtonPressed) { mButtonPressed = false; exit(0); } // quit application
    }
    else glColor3f(0, 0, 0);
    displayRasterText(-100, -170, 0.4f, " Quit");

    // Tell GLUT to redraw again (keeps menu responsive)
    glutPostRedisplay();
}

// Helper to draw a "Back" label and handle clicking it
void backButton() {
    if (mouseX <= -450 && mouseX >= -500 && mouseY >= -275 && mouseY <= -250) {
        glColor3f(0, 0, 1); // blue when hovered
        if (mButtonPressed) { viewPage = MENU; mButtonPressed = false; glutPostRedisplay(); }
    }
    else glColor3f(1, 0, 0); // default red
    displayRasterText(-1000, -550, 0, "Back");
}

// Instructions screen rendering (shows key bindings and rules)
void instructionsScreenDisplay()
{
    // display() already cleared the frame; draw static instructional text
    glColor3f(1, 0, 0);
    displayRasterText(-900, 550, 0.4f, "INSTRUCTIONS");
    glColor3f(1, 1, 1);
    displayRasterText(-1100, 400, 0.4f, "LEFT PLAYER: WASD (move)  - 'c' to shoot (use W/S to aim up/down)");
    displayRasterText(-1100, 320, 0.4f, "RIGHT PLAYER: IJKL (move) - 'm' to shoot (use I/K to aim up/down)");
    displayRasterText(-1100, 240, 0.4f, "Each successful shot deals 10 LIFE points.");
    displayRasterText(-1100, 160, 0.4f, "Laser lasts a brief moment per shot (can't hold to spam).");
    backButton(); // draw back label to return to menu
}

// -----------------------------------------------------------------------------
// Drawing functions for ship parts (composed from vertex arrays above)
// -----------------------------------------------------------------------------

// Draw alien body polygon using predefined vertex list
void DrawAlienBody() {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 8; i++) glVertex2fv(AlienBody[i]); // emit body vertices
    glEnd();

    // Outline
    glColor3f(0, 0, 0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 8; i++) glVertex2fv(AlienBody[i]);
    glEnd();

    // small decorative line
    glBegin(GL_LINES); glVertex2f(-13, 11); glVertex2f(-15, 9); glEnd();
}

// Collar polygon/stroke
void DrawAlienCollar() {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 20; i++) glVertex2fv(AlienCollar[i]);
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) glVertex2fv(AlienCollar[i]);
    glEnd();
}

// Face polygon and outline
void DrawAlienFace() {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 42; i++) glVertex2fv(ALienFace[i]);
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 42; i++) glVertex2fv(ALienFace[i]);
    glEnd();

    // small ear / decoration
    glBegin(GL_LINE_STRIP); glVertex2f(3.3, 22); glVertex2f(4.4, 23.5); glVertex2f(6.3, 26); glEnd();
}

// Beak polygon + outline
void DrawAlienBeak() {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 14; i++) glVertex2fv(ALienBeak[i]);
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 14; i++) glVertex2fv(ALienBeak[i]);
    glEnd();
}

// Eyes: use scaled spheres for effect
void DrawAlienEyes() {
    glPushMatrix();
    glRotated(-10, 0, 0, 1);
    glTranslated(-6, 32.5, 0);
    glScalef(2.5, 4, 0);
    glutSolidSphere(1, 20, 30);
    glPopMatrix();

    glPushMatrix();
    glRotated(-1, 0, 0, 1);
    glTranslated(-8, 36, 0);
    glScalef(2.5, 4, 0);
    glutSolidSphere(1, 100, 100);
    glPopMatrix();
}

// Composite alien drawing: optionally mirror horizontally so pilot faces right
void DrawAlienComposite(bool mirrorForRightFacing) {
    if (mirrorForRightFacing) {
        glPushMatrix();
        glScalef(-1.0f, 1.0f, 1.0f); // flip horizontally
        glColor3f(0, 1, 0); DrawAlienBody();
        glColor3f(1, 0, 0); DrawAlienCollar();
        glColor3f(0, 0, 1); DrawAlienFace();
        glColor3f(1, 1, 0); DrawAlienBeak();
        glColor3f(0, 1, 1); DrawAlienEyes();
        glPopMatrix();
    }
    else {
        // normal (not mirrored)
        glColor3f(0, 1, 0); DrawAlienBody();
        glColor3f(1, 0, 0); DrawAlienCollar();
        glColor3f(0, 0, 1); DrawAlienFace();
        glColor3f(1, 1, 0); DrawAlienBeak();
        glColor3f(0, 1, 1); DrawAlienEyes();
    }
}

/*
    DrawSpaceshipBody:
    - The overall hull is a scaled sphere.
    - Lights (small spheres) are positioned either left (for left ship) or right (for right ship)
      and colored using LightColor[] with index CI to animate colors.
*/
void DrawSpaceshipBody(bool isLeft)
{
    // Base color: red for left, purple-ish for right
    if (isLeft) glColor3f(1, 0, 0); else glColor3f(0.5, 0, 0.5);

    // Large ellipsoid hull made by scaling a unit sphere
    glPushMatrix();
    glScalef(70, 20, 1);
    glutSolidSphere(1, 50, 50);
    glPopMatrix();

    // Draw a row of small lights; use CI as rotating index to animate
    glPushMatrix();
    glScalef(3, 3, 1);
    if (isLeft) {
        // Place lights to the left of center and move rightwards
        glTranslated(-20, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10);
    }
    else {
        // Lights on the right side: start at +20 and move leftwards
        glTranslated(20, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 0) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 1) % 3]); glutSolidSphere(1, 10, 10); glTranslated(-5, 0, 0);
        glColor3fv(LightColor[(CI + 2) % 3]); glutSolidSphere(1, 10, 10);
    }
    glPopMatrix();
}

// Steering wheel (cosmetic)
void DrawSteeringWheel() {
    glPushMatrix();
    glLineWidth(3);
    glColor3f(0.20f, 0., 0.20f);
    glScalef(7, 4, 1);
    glTranslated(-1.9, 5.5, 0);
    glutWireSphere(1, 8, 8);
    glPopMatrix();
}

// Dome above the ship (cosmetic)
void DrawSpaceshipDome() {
    glColor4f(0.7f, 1, 1, 0.0011f);
    glPushMatrix();
    glTranslated(0, 30, 0);
    glScalef(35, 50, 1);
    glutSolidSphere(1, 50, 50);
    glPopMatrix();
}

// Draw a laser as a single thick red line between two points
void DrawLaser(int x, int y, int xend, int yend) {
    glLineWidth(5);
    glColor3f(1, 0, 0);
    glBegin(GL_LINES);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)xend, (float)yend);
    glEnd();
}

// Construct a spaceship at position (x,y). isLeft indicates if it's the left-side ship.
void SpaceshipCreate(float x, float y, bool isLeft) {
    glPushMatrix();
    glTranslated(x, y, 0);
    DrawSpaceshipDome();

    // The pilot sits inside dome and may be mirrored to face right for left ship.
    glPushMatrix();
    glTranslated(4, 19, 0);
    if (isLeft) {
        DrawAlienComposite(true);  // mirrored so pilot appears to face right
    }
    else {
        DrawAlienComposite(false);
    }
    glPopMatrix();

    DrawSteeringWheel();
    DrawSpaceshipBody(isLeft);
    glPopMatrix();
}

// -----------------------------------------------------------------------------
// HUD: Health and scoreboard display
// -----------------------------------------------------------------------------

void DisplayHealthLeft() {
    char buf[40];
    snprintf(buf, sizeof(buf), "LIFE = %d", lifeLeft); // format left life text
    glColor3f(1, 1, 1);
    displayRasterText(-1100, 600, 0.4f, buf); // draw at top-left world coordinate
}

void DisplayHealthRight() {
    char buf[40];
    snprintf(buf, sizeof(buf), "LIFE = %d", lifeRight);
    glColor3f(1, 1, 1);
    displayRasterText(800, 600, 0.4f, buf); // top-right HUD
}

// -----------------------------------------------------------------------------
// Geometry utility: compute squared distance from point P(px,py) to segment AB.
// Returns squared distance to avoid sqrt when comparing to r*r.
// -----------------------------------------------------------------------------
float pointToSegmentDist2(float px, float py, float ax, float ay, float bx, float by) {
    float vx = bx - ax, vy = by - ay;           // AB vector
    float wx = px - ax, wy = py - ay;           // AP vector
    float vv = vx * vx + vy * vy;               // squared length of AB
    if (vv == 0.0f) {                            // AB is a point
        float dx = px - ax, dy = py - ay;
        return dx * dx + dy * dy;
    }
    // Projection parameter t along AB where closest point lies (unclamped)
    float t = (wx * vx + wy * vy) / vv;
    if (t < 0.0f) t = 0.0f;                      // clamp to A
    if (t > 1.0f) t = 1.0f;                      // clamp to B
    float cx = ax + t * vx, cy = ay + t * vy;    // closest point coordinates
    float dx = px - cx, dy = py - cy;            // vector from closest point to P
    return dx * dx + dy * dy;                    // squared distance
}

// -----------------------------------------------------------------------------
// Collision: check if laser segment intersects opponent's circular ship area.
// Applies damage only once per beam and awards score to shooter.
// -----------------------------------------------------------------------------
void checkLaserHitAndApply(int x0, int y0, int x1, int y1,
    float shipX, float shipY,
    int* shipLife, bool* laserFlag, bool* laserHasHit, bool shooterIsLeft) {

    const float r = 50.0f; // approximate ship radius (world units)

    // If this beam already applied its damage, skip
    if (*laserHasHit) return;

    // Offset ship center slightly by (8,8) to match visual alignment in original code
    float d2 = pointToSegmentDist2(shipX + 8.0f, shipY + 8.0f, (float)x0, (float)y0, (float)x1, (float)y1);

    if (d2 <= r * r) {
        // Hit: apply damage and clamp
        *shipLife -= 10;      // subtract fixed HP per hit
        if (*shipLife < 0) *shipLife = 0;
        *laserHasHit = true;  // mark as having applied damage

        // Award points to shooter (simple scoreboard logic)
        if (shooterIsLeft) scoreLeft += 100;
        else scoreRight += 100;

        // Keep laser visible until its timer runs out (do not turn off here)
    }
}

// -----------------------------------------------------------------------------
// Keep ship positions inside generous bounds to avoid them wandering infinitely.
// -----------------------------------------------------------------------------
void clampPositions() {
    float limitX = 1000.0f;
    float limitY = 300.0f;
    if (xLeft > limitX) xLeft = limitX; if (xLeft < -limitX) xLeft = -limitX;
    if (yLeft > limitY) yLeft = limitY; if (yLeft < -limitY) yLeft = -limitY;
    if (xRight > limitX) xRight = limitX; if (xRight < -limitX) xRight = -limitX;
    if (yRight > limitY) yRight = limitY; if (yRight < -limitY) yRight = -limitY;
}

// -----------------------------------------------------------------------------
// Game screen rendering: draws HUD, ships, lasers, and checks collisions.
// Note: this function performs a glScalef(2,2,1) in the original; keep consistent.
// -----------------------------------------------------------------------------
void gameScreenDisplay()
{
    // Draw health and score HUD
    DisplayHealthLeft();
    DisplayHealthRight();

    // Score text
    char sb[64];
    snprintf(sb, sizeof(sb), "SCORE = %d", scoreLeft);
    glColor3f(1, 1, 1);
    displayRasterText(-1100, 560, 0.4f, sb); // left score
    snprintf(sb, sizeof(sb), "SCORE = %d", scoreRight);
    displayRasterText(800, 560, 0.4f, sb); // right score

    // Apply a global scale for the game view (original design)
    glScalef(2.0f, 2.0f, 1.0f);

    // LEFT ship: drawn at -xLeft so that positive xLeft moves the visual ship leftwards
    if (lifeLeft > 0) {
        SpaceshipCreate(-xLeft, yLeft, true); // create left ship (mirrored pilot)
        if (laserLeft) {
            // Compute laser endpoints: laser from ship toward +XMAX (rightwards)
            int xs = (int)roundf(-xLeft), ys = (int)roundf(yLeft);
            int xe = XMAX, ye = ys; // default horizontal
            if (laserLeftDir[0]) ye = YMAX;      // aim up if specified
            else if (laserLeftDir[1]) ye = -YMAX; // aim down if specified

            DrawLaser(xs, ys, xe, ye);

            // Check collision against right ship located at +xRight
            // shooterIsLeft = true (award left player's score if hit)
            checkLaserHitAndApply(xs, ys, xe, ye, xRight, yRight, &lifeRight, &laserLeft, &laserLeftHasHit, true);
        }
    }
    else {
        // Left ship dead -> move to gameover
        viewPage = GAMEOVER;
    }

    // RIGHT ship: drawn at +xRight
    if (lifeRight > 0) {
        SpaceshipCreate(xRight, yRight, false);
        if (laserRight) {
            // Laser from right ship goes leftwards toward -XMAX
            int xs = (int)roundf(xRight), ys = (int)roundf(yRight);
            int xe = -XMAX, ye = ys;
            if (laserRightDir[0]) ye = YMAX;
            else if (laserRightDir[1]) ye = -YMAX;

            DrawLaser(xs, ys, xe, ye);

            // Check collision against left ship at -xLeft
            checkLaserHitAndApply(xs, ys, xe, ye, -xLeft, yLeft, &lifeLeft, &laserRight, &laserRightHasHit, false);
        }
    }
    else {
        // Right ship dead -> gameover
        viewPage = GAMEOVER;
    }

    if (viewPage == GAMEOVER) {
        // We intentionally do not reset lives here; displayGameOverMessage uses life values to decide winner
    }
}

// Display the game-over message and final scores
void displayGameOverMessage() {
    glColor3f(1, 1, 0); // yellow text
    const char* msg;
    if (lifeLeft > 0 && lifeRight <= 0) msg = "Game Over! Left player won";
    else if (lifeRight > 0 && lifeLeft <= 0) msg = "Game Over! Right player won";
    else msg = "Game Over!";
    displayRasterText(-350, 600, 0.4f, msg);

    // Prompt for returning to menu
    displayRasterText(-350, 500, 0.4f, "Press ENTER to return to menu");

    // Final score summary
    char fb[80];
    snprintf(fb, sizeof(fb), "Final Score — Left: %d   Right: %d", scoreLeft, scoreRight);
    displayRasterText(-350, 460, 0.4f, fb);
}

// -----------------------------------------------------------------------------
// Input handling & game logic
// Handles ENTER to change screens and per-frame controls while in GAME.
// -----------------------------------------------------------------------------
void keyOperations() {
    // ENTER key (ASCII 13) transitions from INTRO -> MENU or from GAMEOVER -> MENU (and resets)
    if (keyStates[13]) {
        if (viewPage == INTRO) {
            viewPage = MENU;
        }
        else if (viewPage == GAMEOVER) {
            // Reset game state and return to menu
            lifeLeft = lifeRight = 100;
            laserLeft = laserRight = false;
            laserLeftHasHit = laserRightHasHit = false;
            laserLeftTimer = laserRightTimer = 0.0f;
            xLeft = xRight = 500.0f; yLeft = yRight = 0.0f;
            scoreLeft = scoreRight = 0;
            viewPage = MENU;
        }
    }

    // Per-frame controls are only processed while in GAME state
    if (viewPage == GAME) {
        // Reset per-frame aim flags
        laserLeftDir[0] = laserLeftDir[1] = false;
        laserRightDir[0] = laserRightDir[1] = false;

        // ---------------- LEFT PLAYER (WASD + c to shoot) ----------------
        if (keyStates['c']) {
            // If canShootLeft is true, start a new laser (one-shot per press)
            if (canShootLeft) {
                laserLeft = true;
                laserLeftTimer = LASER_DURATION_SECONDS; // set visibility timer
                laserLeftHasHit = false;                 // reset hit marker for this beam
                canShootLeft = false;                    // require release before next shot
            }
            // While holding 'c', allow W/S to adjust the beam direction
            if (keyStates['w']) laserLeftDir[0] = true;
            if (keyStates['s']) laserLeftDir[1] = true;
        }
        else {
            // If 'c' is not pressed, allow shooting again on next press
            canShootLeft = true;

            // Movement while not holding shoot: move according to deltaTime so speed is independent of FPS
            float move = SPACESHIP_SPEED * deltaTime;

            // NOTE: left ship is drawn at -xLeft for mirrored layout; to make 'd' move right visually we DECREASE xLeft.
            if (keyStates['d']) xLeft -= move;    // D key -> visually move right
            if (keyStates['a']) xLeft += move;    // A key -> visually move left
            if (keyStates['w']) yLeft += move;    // W -> up
            if (keyStates['s']) yLeft -= move;    // S -> down
        }

        // ---------------- RIGHT PLAYER (IJKL + m to shoot) ----------------
        if (keyStates['m']) {
            if (canShootRight) {
                laserRight = true;
                laserRightTimer = LASER_DURATION_SECONDS;
                laserRightHasHit = false;
                canShootRight = false;
            }
            if (keyStates['i']) laserRightDir[0] = true;
            if (keyStates['k']) laserRightDir[1] = true;
        }
        else {
            canShootRight = true;
            float move = SPACESHIP_SPEED * deltaTime;
            if (keyStates['l']) xRight += move;
            if (keyStates['j']) xRight -= move;
            if (keyStates['i']) yRight += move;
            if (keyStates['k']) yRight -= move;
        }

        // Ensure ships remain within allowed bounds
        clampPositions();
    }
}

// -----------------------------------------------------------------------------
// Main display callback (single swap here to avoid flicker).
// Calls appropriate renderer depending on current viewPage state.
// -----------------------------------------------------------------------------
void display()
{
    keyOperations();                     // process keyboard-based logic first
    glClear(GL_COLOR_BUFFER_BIT);        // clear for new frame

    switch (viewPage) {
    case INTRO:
        introScreen();
        break;
    case MENU:
        startScreenDisplay();
        break;
    case INSTRUCTIONS:
        instructionsScreenDisplay();
        break;
    case GAME:
        gameScreenDisplay();
        // Original code undid scaling by scaling down; leave that behavior
        glScalef(0.5f, 0.5f, 1.0f);
        break;
    case GAMEOVER:
        // Render final scene and overlay gameover text
        gameScreenDisplay();
        glScalef(0.5f, 0.5f, 1.0f);
        displayGameOverMessage();
        break;
    }

    glFlush();        // ensure drawing commands are executed
    glLoadIdentity(); // reset modelview to avoid leaking transforms into next frame

    // Single buffer swap (double buffering used in main) — reduces flicker
    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// Input callbacks: mouse/motion/keyboard
// -----------------------------------------------------------------------------

// Convert from window pixel coords to world coords using cached m_viewport and world extents
void passiveMotionFunc(int x, int y) {
    if (m_viewport[2] != 0 && m_viewport[3] != 0) {
        // Map pixel x to [-600..600] and pixel y to [-350..350] world coordinates
        mouseX = (float)x / (m_viewport[2] / 1200.0f) - 600.0f;
        mouseY = -((float)y / (m_viewport[3] / 700.0f) - 350.0f);
    }
    glutPostRedisplay(); // request redraw to update hover highlights
}

// Mouse click handling (left button)
void mouseClick(int buttonPressed, int state, int x, int y) {
    if (buttonPressed == GLUT_LEFT_BUTTON && state == GLUT_DOWN) mButtonPressed = true;
    else mButtonPressed = false;
    glutPostRedisplay();
}

// Keyboard down: mark key pressed in keyStates[] and request redraw
void keyPressed(unsigned char key, int x, int y) { keyStates[key] = true; glutPostRedisplay(); }

// Keyboard up: mark key released
void keyReleased(unsigned char key, int x, int y) { keyStates[key] = false; glutPostRedisplay(); }

// -----------------------------------------------------------------------------
// Idle / refresh function (runs when GLUT is idle) — used for time-based updates
// -----------------------------------------------------------------------------
void refresh() {
    // Compute deltaTime (seconds) since last frame using GLUT elapsed time
    int now = glutGet(GLUT_ELAPSED_TIME);
    int diff = now - lastTimeMs;
    if (diff < 0) diff = 0;
    deltaTime = diff / 1000.0f;
    lastTimeMs = now;

    // Animate the colored lights only while in GAME view
    static int lightCounter = 0;
    if (viewPage == GAME) {
        lightCounter++;
        if (lightCounter >= 8) { CI = (CI + 1) % 3; lightCounter = 0; } // slowly rotate CI
    }

    // Decrement timers for visible lasers; when timer expires, turn off beam and reset hit marker.
    if (laserLeft) {
        laserLeftTimer -= deltaTime;
        if (laserLeftTimer <= 0.0f) {
            laserLeft = false;
            laserLeftTimer = 0.0f;
            laserLeftHasHit = false; // allow future beams to hit again
        }
    }
    if (laserRight) {
        laserRightTimer -= deltaTime;
        if (laserRightTimer <= 0.0f) {
            laserRight = false;
            laserRightTimer = 0.0f;
            laserRightHasHit = false;
        }
    }

    // Request re-display at end of idle processing
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
// main: initialize GLUT and register callbacks
// -----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    // Use double-buffering to avoid flicker and RGB color mode
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    // Initial window position and size
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(1200, 600);

    // Create the window with a title
    glutCreateWindow("Space Shooter - with scoreboard");

    init(); // set up projection, clear color, and timing baseline

    // Register callbacks
    glutIdleFunc(refresh);                 // called when GLUT is idle (used for timing)
    glutKeyboardFunc(keyPressed);          // key down
    glutKeyboardUpFunc(keyReleased);       // key up
    glutMouseFunc(mouseClick);             // mouse click
    glutPassiveMotionFunc(passiveMotionFunc); // mouse moved without button pressed
    glutDisplayFunc(display);              // main display callback

    // Retrieve the viewport (x,y,width,height) for converting mouse coords
    glGetIntegerv(GL_VIEWPORT, m_viewport);

    // Enter GLUT event loop — this function never returns
    glutMainLoop();
    return 0;
}
