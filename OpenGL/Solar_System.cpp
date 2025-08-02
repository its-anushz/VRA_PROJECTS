//1
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#include <ctime>
#include <cstdio>

#define PI 3.14159265358979323846
const int STAR_COUNT = 500;
float stars[STAR_COUNT][3];

bool showRestartMessage = false;
int restartMessageTimer = 0;


// Orbital Radii
int mercuryRadius = 200;
int venusRadius = mercuryRadius + 150;
int earthRadius = venusRadius + 150;
int marsRadius = earthRadius + 150;
int jupiterRadius = marsRadius + 200;
int saturnRadius = jupiterRadius + 200;
int uranusRadius = saturnRadius + 150;
int neptuneRadius = uranusRadius + 150;

// Speeds and Angles
float mercury = 360.0f / 87.97f, venus = 360.0f / 224.7f, earth = 1.0f;
float mars = 360.0f / 686.98f, jupiter = 360.0f / 4332.82f;
float saturn = 360.0f / 10755.7f, uranus = 360.0f / 30687.15f;
float neptune = 360.0f / 60190.03f;

float mercuryAngle = 0, venusAngle = 0, earthAngle = 0;
float marsAngle = 0, jupiterAngle = 0, saturnAngle = 0;
float uranusAngle = 0, neptuneAngle = 0;

// Camera & comet state
float eyeX = 0.0f, eyeY = 1200.0f, eyeZ = 2600.0f;
float zoomTarget = 1500.0f, zoomSpeed = 1.5f;
float cometX = -3000.0f, cometY = 500.0f, cometZ = -3000.0f;
bool cometMoving = true;

// Explosion state
float earthX = 0.0f, earthZ = 0.0f;
bool freezeOrbits = false, showExplosion = false, earthDamaged = false;
int explosionTimer = 0, restartTimer = 0;

// Scene phase enum
enum ScenePhase { CALM, COMET_APPROACH, IMPACT, AFTERMATH };
ScenePhase currentPhase = CALM;

// Lighting
GLfloat mat_specular[] = { 0.2, 0.2, 0.2, 1.0 };
GLfloat mat_shininess[] = { 50.0 };
GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };
GLfloat model_ambient[] = { 0.3, 0.3, 0.3, 1.0 };

// Declarations
void changeColor(float, float, float, float);
void setupMaterials();
void drawOrbit(float);
void drawStars();
void drawText(const char*, float, float);
void drawComet();
void updatePlanets();
void renderScene();
void keyboard(unsigned char, int, int);
void mouse(int, int, int, int);
void init();
void changeSize(int, int);


// 2
void changeColor(float r, float g, float b, float a) {
    GLfloat color[] = { r, g, b, a };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color);
}

void setupMaterials() {
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, model_ambient);
}

void drawText(const char* text, float x, float y) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, 1200, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    glRasterPos2f(x, y);
    for (const char* c = text; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float angle = 2 * PI * i / 100;
        glVertex3f(cos(angle) * radius, 0.0f, sin(angle) * radius);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    glColor3f(1.0, 1.0, 1.0);
    for (int i = 0; i < STAR_COUNT; ++i)
        glVertex3f(stars[i][0], stars[i][1], stars[i][2]);
    glEnd();
    glEnable(GL_LIGHTING);
}

void init() {
    glClearColor(0.0, 0.0, 0.0, 1.0);
    setupMaterials();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glShadeModel(GL_SMOOTH);

    srand(static_cast<unsigned int>(time(0)));
    for (int i = 0; i < STAR_COUNT; ++i) {
        stars[i][0] = ((rand() % 4000) - 2000);
        stars[i][1] = ((rand() % 2000) - 1000);
        stars[i][2] = ((rand() % 4000) - 2000);
    }
}

void changeSize(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / (float)h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45, ratio, 1, 5000);
    glMatrixMode(GL_MODELVIEW);
}
//3
void drawComet() {
    if (currentPhase == COMET_APPROACH || currentPhase == IMPACT) {
        glPushMatrix();
        glTranslatef(cometX, cometY, cometZ);
        // Main Comet Head
        GLfloat cometColor[] = { 0.6f, 0.9f, 1.0f, 1.0f }; // Icy blue
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cometColor);
        gluSphere(gluNewQuadric(), 20, 30, 30);

        // Tail using translucent spheres behind the comet
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (int i = 1; i <= 8; ++i) {
            glPushMatrix();
            float offset = -i * 25.0f;  // Tail spacing
            glTranslatef(offset, 0.0f, offset);  // Trailing back diagonally
            GLfloat tailColor[] = { 0.6f, 0.9f, 1.0f, 1.0f - (i * 0.1f) };  // Fading transparency
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, tailColor);
            gluSphere(gluNewQuadric(), 10 - i * 0.7, 16, 16);  // Shrinking size
            glPopMatrix();
        }

        glDisable(GL_BLEND);
    }
}
void updatePlanets() {
    if (!freezeOrbits) {
        mercuryAngle += mercury;
        venusAngle += venus;
        earthAngle += earth;
        marsAngle += mars;
        jupiterAngle += jupiter;
        saturnAngle += saturn;
        uranusAngle += uranus;
        neptuneAngle += neptune;
    }

    static float cometSpeed = 2.5f;
    if (currentPhase == CALM && glutGet(GLUT_ELAPSED_TIME) > 3000)
        currentPhase = COMET_APPROACH;

    if (currentPhase == COMET_APPROACH && eyeZ > zoomTarget) {
        eyeZ -= zoomSpeed;
        if (eyeZ <= zoomTarget + 50)
            freezeOrbits = true;
    }

    if (currentPhase == COMET_APPROACH && cometMoving) {
        float dx = earthX - cometX;
        float dy = 0.0f - cometY;
        float dz = earthZ - cometZ;
        float dist = sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 40.0f) {
            cometMoving = false;
            currentPhase = IMPACT;
            showExplosion = true;
            explosionTimer = 40;
            earthDamaged = true;
        }
        else {
            dx /= dist; dy /= dist; dz /= dist;
            cometX += dx * cometSpeed;
            cometY += dy * cometSpeed;
            cometZ += dz * cometSpeed;
        }
    }

    if (currentPhase == IMPACT && explosionTimer > 0) {
        explosionTimer--;
        if (explosionTimer == 0)
            currentPhase = AFTERMATH;
    }

    if (currentPhase == AFTERMATH) {
        if (eyeZ < 2600.0f)
            eyeZ += 1.0f;

        restartTimer++;
        if (restartTimer > 200) {
            freezeOrbits = false;
            earth = 0.3f;
        }
    }

    glutPostRedisplay();
}
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    drawStars();

    drawOrbit(mercuryRadius);
    drawOrbit(venusRadius);
    drawOrbit(earthRadius);
    drawOrbit(marsRadius);
    drawOrbit(jupiterRadius);
    drawOrbit(saturnRadius);
    drawOrbit(uranusRadius);
    drawOrbit(neptuneRadius);

    // Sun
    changeColor(1.0f, 1.0f, 0.0f, 1.0f);
    GLUquadric* sun = gluNewQuadric();
    gluSphere(sun, 100, 100, 100);

    // Compute Earth position (used for comet targeting)
    float earthAngleRad = earthAngle * PI / 180.0f;
    earthX = freezeOrbits ? cos(0) * earthRadius : cos(earthAngleRad) * earthRadius;
    earthZ = freezeOrbits ? sin(0) * earthRadius : sin(earthAngleRad) * earthRadius;

    // Mercury
    glPushMatrix();
    glRotatef(mercuryAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(mercuryRadius, 0.0, 0.0);
    changeColor(0.6f, 0.6f, 0.6f, 1.0f);
    gluSphere(gluNewQuadric(), 10, 100, 100);
    glPopMatrix();

    // Venus
    glPushMatrix();
    glRotatef(venusAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(venusRadius, 0.0, 0.0);
    changeColor(1.0f, 1.0f, 0.0f, 1.0f);
    gluSphere(gluNewQuadric(), 25.8, 100, 100);
    glPopMatrix();

    // Earth
    glPushMatrix();
    glRotatef(freezeOrbits ? 0.0f : earthAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(earthRadius, 0.0, 0.0);
    if (!earthDamaged)
        changeColor(0.0f, 1.0f, 0.0f, 1.0f);
    else
        changeColor(0.0f, 0.0f, 1.0f, 1.0f);
    gluSphere(gluNewQuadric(), 20.6, 100, 100);
    glPopMatrix();

    // Mars
    glPushMatrix();
    glRotatef(marsAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(marsRadius, 0.0, 0.0);
    changeColor(1.0f, 0.3f, 0.3f, 1.0f);
    gluSphere(gluNewQuadric(), 18.4, 100, 100);
    glPopMatrix();

    // Jupiter
    glPushMatrix();
    glRotatef(jupiterAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(jupiterRadius, 0.0, 0.0);
    changeColor(1.0f, 0.6f, 0.2f, 1.0f);
    gluSphere(gluNewQuadric(), 64.5, 100, 100);
    glPopMatrix();

    // Saturn
    // Saturn
    glPushMatrix();
    glRotatef(saturnAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(saturnRadius, 0.0f, 0.0f);
    changeColor(1.0f, 0.9f, 0.6f, 1.0f);  // yellowish
    glutSolidSphere(38, 20, 20);
    glPopMatrix();

    // Uranus
    glPushMatrix();
    glRotatef(uranusAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(uranusRadius, 0.0, 0.0);
    changeColor(0.3f, 1.0f, 0.9f, 1.0f);
    gluSphere(gluNewQuadric(), 22.5, 100, 100);
    glPopMatrix();

    // Neptune
    glPushMatrix();
    glRotatef(neptuneAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(neptuneRadius, 0.0, 0.0);
    changeColor(0.3f, 0.5f, 1.0f, 1.0f);
    gluSphere(gluNewQuadric(), 24.0, 100, 100);
    glPopMatrix();

    // Comet
    drawComet();

    // Explosion
    if (showExplosion && explosionTimer > 0) {
        glPushMatrix();
        glTranslatef(earthX, 0.0f, earthZ);
        changeColor(1.0f, 1.0f, 0.0f, 1.0f);
        glutSolidSphere(40.0, 30, 30);
        glPopMatrix();
    }

    // Final text
    if (currentPhase == AFTERMATH && explosionTimer == 0) {
        drawText("Earth... has changed forever.", 30, 50);
    }

    // Show restart message if active
    if (showRestartMessage) {
        drawText("Simulation Restarted!", 500, 750);
        restartMessageTimer--;
        if (restartMessageTimer <= 0)
            showRestartMessage = false;
    }


    glutSwapBuffers();
}
void keyboard(unsigned char key, int, int) {
    if (key == 27) exit(0); // ESC to exit
    if (key == 'w' || key == 'W') eyeZ -= 50.0f;
    if (key == 's' || key == 'S') eyeZ += 50.0f;
    if (key == 'a' || key == 'A') eyeX -= 50.0f;
    if (key == 'd' || key == 'D') eyeX += 50.0f;

    if (key == 'r' || key == 'R') {
        // Reset simulation
        currentPhase = CALM;
        eyeX = 0.0f; eyeY = 1200.0f; eyeZ = 2600.0f;
        cometX = -3000.0f; cometY = 500.0f; cometZ = -3000.0f;
        cometMoving = true;
        showExplosion = false;
        earthDamaged = false;
        freezeOrbits = false;
        explosionTimer = 0;
        restartTimer = 0;
        earth = 1.0f;
        earthAngle = 0;

        // Show restart message
        showRestartMessage = true;
        restartMessageTimer = 100;
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int, int) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
        glutIdleFunc(updatePlanets);  // Start animation
    else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
        glutIdleFunc(NULL);  // Pause animation
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Solar System - Halley's Comet Story");

    init();
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutIdleFunc(updatePlanets);
    glutMainLoop();

    return 0;
}
