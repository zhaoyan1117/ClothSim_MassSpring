#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmath>
#include <sys/time.h>
#include <time.h>

#include <GL/glew.h>
#include <glm/glm.hpp>

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include "FreeImage.h"
#include "ClothSim.h"

const int FRAME_TIME = 5;

using namespace std;

class Viewport {
  public:
    int w, h; // width and height
    int px, py;
};

// Global Variables.
short mode = 0;
Cloth* cloth;
Viewport viewport, fullScreen;
bool isLine = false;
bool isFullScreen = false;
bool isWind = false;
bool isBall = false;
bool showForce = false;

bool isPaused = false;
bool outputImage = false;
int curFrame = 0;

float size01 = 0.08f;
// Cloth0, for wind blow test.
float ks0 = 30.0f;
float kd0 = 15.0f;
Cloth cloth0 = Cloth(14.0f, 10.0f, 55, 45, ks0, kd0, false);

// Cloth1, for ball obstacle test.
float ks1 = 30.0f;
float kd1 = 15.0f;
Cloth cloth1 = Cloth(14.0f, 10.0f, 55, 45, ks1, kd1, false);
glm::vec3 stepX = glm::vec3(0.2f, 0.0f, 0.0f);
glm::vec3 stepZ = glm::vec3(0.0f, 0.0f, 0.2f);
glm::vec3 ballCenter = glm::vec3(7.0f, -5.0f, 3.0f);
float ballRadius = 2.0f;

float size23456 = 0.3f;
// Cloth2, for drop test with ball.
float ks2 = 20.0f;
float kd2 = 10.0f;
int ww = 40; int hh = 40;
std::vector<Particle> pp = creatParticles(ww, hh, 40.0f, 40.0f);
Cloth cloth2 = Cloth(pp, ww, hh, ks2, kd2, true);
float cloth2Floor = -35.0f;
glm::vec3 holderBallCenter = glm::vec3(19.5f, -20.0f, 19.5f);
float holderBallRadius = 10.0f;

// Cloth3, for drop test with cube.
float ks3 = 20.0f;
float kd3 = 10.0f;
int wwc = 40; int hhc = 40;
std::vector<Particle> ppc = creatParticles(wwc, hhc, 40.0f, 40.0f);
Cloth cloth3 = Cloth(ppc, wwc, hhc, ks3, kd3, true);
float cloth3Floor = -30.0f;
glm::vec3 cubeCenter = glm::vec3(20.0f, -20.0f, 20.0f);
float cubeH = 11.1f;
glm::vec3 cubeMin = glm::vec3(cubeCenter.x-cubeH, -30.0, cubeCenter.z-cubeH);
glm::vec3 cubeMax = glm::vec3(cubeCenter.x+cubeH, -10.0, cubeCenter.z+cubeH);

// Cloth4, for drop test on table (big cube).
float ks4 = 20.0f;
float kd4 = 10.0f;
int wwt = 40; int hht = 40;
std::vector<Particle> ppt = creatParticles(wwt, hht, 40.0f, 40.0f);
Cloth cloth4 = Cloth(ppt, wwt, hht, ks4, kd4, true);
float cloth4Floor = -30.0f;
glm::vec3 tableCenter = glm::vec3(20.0f, -20.0f, 20.0f);
float tableH = 25.0f;
glm::vec3 tableMin = glm::vec3(tableCenter.x-tableH, -25.0, tableCenter.z-tableH);
glm::vec3 tableMax = glm::vec3(tableCenter.x+tableH, -15.0, tableCenter.z+tableH);

// Cloth5, for drop test on stick (long cube).
float ks5 = 20.0f;
float kd5 = 10.0f;
int wws = 40; int hhs = 40;
std::vector<Particle> pps = creatParticles(wws, hhs, 40.0f, 40.0f);
Cloth cloth5 = Cloth(pps, wws, hhs, ks5, kd5, true);
float cloth5Floor = -30.0f;
glm::vec3 stickCenter = glm::vec3(20.0f, -20.0f, 20.0f);
float stickH = 1.1f;
glm::vec3 stickMin = glm::vec3(stickCenter.x-stickH, -25.0, stickCenter.z-stickH);
glm::vec3 stickMax = glm::vec3(stickCenter.x+stickH, -15.0, stickCenter.z+stickH);

// Cloth6, for drop test on the floor.
float ks6 = 10.0f;
float kd6 = 5.0f;
int wwf = 40; int hhf = 40;
std::vector<Particle> ppf = creatParticles(wwf, hhf, 40.0f, 40.0f);
Cloth cloth6 = Cloth(ppf, wwf, hhf, ks6, kd6, true);
float cloth6Floor = -25.0f;

void setCamera(int w, int h) {
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
    gluPerspective(80, (float)w/(float)h, 1.0, 5000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (mode == 0 or mode == 1) {
        glTranslatef(-6.5f, 6.0f, -9.0f);
        glRotatef(25,0,1,0);
    } else if (mode == 2 or mode == 3 or mode == 4 or mode == 5 or mode == 6) {
        gluLookAt(20.0f, 5.0f, 60.0f, 20.0f, -25.0f, 20.0f, 0.0f, 1.0f, 0.0f);
    }
}

void chooseCloth() {
    if (mode == 0) {
        cloth = &cloth0;
    } else if (mode == 1) {
        cloth = &cloth1;
    } else if (mode == 2) {
        cloth = &cloth2;
    } else if (mode == 3) {
        cloth = &cloth3;
    } else if (mode == 4) {
        cloth = &cloth4;
    } else if (mode == 5) {
        cloth = &cloth5;
    } else if (mode == 6) {
        cloth = &cloth6;
    }
}

void myReshape(int w, int h) {
    if (!isFullScreen) {
        viewport.w = w; viewport.h = h;
    } else {
        fullScreen.w = w; fullScreen.h = h;
    }

    glViewport(0, 0, w, h);
    setCamera(w, h);
}

void initScene(){
    glShadeModel(GL_SMOOTH);
    glClearColor(0.1f, 0.1f, 0.1f, 0.0f);               
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    
    glEnable(GL_COLOR_MATERIAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    glEnable(GL_LIGHTING);

    glEnable(GL_LIGHT1);

    float lightPos[4] = {1.0,0.0,-0.2,0.0};
    float lightDiffuse[4] = {0.5,0.5,0.3,0.0};
    glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION, lightPos);
    glLightfv(GL_LIGHT1,GL_DIFFUSE, lightDiffuse);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);

    chooseCloth();
}

void simulate(Cloth& c) {
    // Gravity and Aerodynamic force.
    c.addForce(glm::vec3(0,-0.1,0));
    if (isWind) { 
        c.updateWindForce(glm::vec3(0.02f, 0.0f, 0.025f)); 
    }

    // Simulate cloth.
    if (mode == 1) {
        c.withBall(ballCenter, ballRadius);
    } else if (mode == 2) {
        c.addForce(glm::vec3(0,-0.1,0));
        c.withBall(holderBallCenter, holderBallRadius);
        c.setFloor(cloth2Floor);
    } else if (mode == 3) {
        c.addForce(glm::vec3(0,-0.1,0));
        c.withCube(cubeMin, cubeMax);
        c.setFloor(cloth3Floor);
    } else if (mode == 4) {
        c.addForce(glm::vec3(0,-0.1,0));
        c.withCube(tableMin, tableMax);
        c.setFloor(cloth4Floor);
    } else if (mode == 5) {
        c.addForce(glm::vec3(0,-0.1,0));
        c.withCube(stickMin, stickMax);
        c.setFloor(cloth5Floor);
    } else if (mode == 6) {
        c.addForce(glm::vec3(0,-0.1,0));
        c.setFloor(cloth6Floor);
    }
    c.timeStep();
}

void drawFloor(float floorY) {
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.1f, 0.1f, 0.1f); glVertex3f(-1200.0f, floorY-0.1, -1200.0f);
    glColor3f(0.7f, 0.9f, 0.6f); glVertex3f(-1200.0f, floorY-0.1, 1200.0f);
    glColor3f(0.7f, 0.9f, 0.6f); glVertex3f(1200.0f, floorY-0.1, 1200.0f);
    glColor3f(0.7f, 0.9f, 0.6f); glVertex3f(1200.0f, floorY-0.1, 1200.0f);
    glColor3f(0.1f, 0.1f, 0.1f); glVertex3f(1200.0f, floorY-0.1, -1200.0f);
    glColor3f(0.1f, 0.1f, 0.1f); glVertex3f(-1200.0f, floorY-0.1, -1200.0f);
    glEnd();
}

void drawOtherStuff() {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    if (mode == 1) {
        glTranslatef(ballCenter.x, ballCenter.y, ballCenter.z);
        glColor3f(0.52f, 0.80f, 0.98f);
        glutSolidSphere(ballRadius-0.1,50,50);
    } else if (mode == 2) {
        // Draw floor.
        drawFloor(cloth2Floor);
        // Draw ball.
        glTranslatef(holderBallCenter.x, holderBallCenter.y, holderBallCenter.z);
        glColor3f(0.52f, 0.80f, 0.98f);
        glutSolidSphere(holderBallRadius-0.1,50,50);
    } else if (mode == 3) {
        // Draw floor.
        drawFloor(cloth3Floor);
        // Draw cube.
        glTranslatef(20.0f, -20.0f, 20.0f);
        glScalef(cubeH/10.0f, 1.0f, cubeH/10.0f);
        glScalef(0.99f, 0.99f, 0.99f);
        glColor3f(0.52f, 0.80f, 0.98f);
        glutSolidCube(20.0f);
    } else if (mode == 4) {
        // Draw floor.
        drawFloor(cloth4Floor);
        // Draw cube.
        glTranslatef(20.0f, -20.0f, 20.0f);
        glScalef(tableH/5.0f, 1.0f, tableH/5.0f);
        glScalef(0.99f, 0.99f, 0.99f);
        glColor3f(0.52f, 0.80f, 0.98f);
        glutSolidCube(10.0f);     
    } else if (mode == 5) {
        // Draw floor.
        drawFloor(cloth5Floor);
        // Draw cube.
        glTranslatef(20.0f, -20.0f, 20.0f);
        glScalef(stickH/5.0f, 1.0f, stickH/5.0f);
        glScalef(0.95f, 0.95f, 0.95f);
        glColor3f(0.52f, 0.80f, 0.98f);
        glutSolidCube(10.0f);     
    } else if (mode == 6) {
        // Draw floor.
        drawFloor(cloth6Floor);   
    }
    glPopMatrix();
}

void writeImage() {
    FreeImage_Initialise ();

    string name;
    ostringstream convert;
    convert << "images/"<< curFrame << ".bmp";
    name = convert.str();

    int width, height;
    width = viewport.w; height = viewport.h;

    BYTE* pixels = new BYTE[ 3 * width * height];

    glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, width, height, 3 * width, 24, 0x0000FF, 0xFF0000, 0x00FF00, false);

    FreeImage_Save(FIF_BMP, image, name.c_str(), 0);
    
    delete image;
    delete pixels;
    FreeImage_DeInitialise (); //Cleanup!
}

void myDisplay() {

    curFrame += 1;

    // Choose cloth.
    float size;
    if (mode == 0 or mode == 1) {
        size = size01;
    } else if (mode == 2 or mode == 3 or mode == 4 or mode == 5 or mode == 6) {
        size = size23456;
    }

    // Simulate cloth.
    if (!isPaused) {
        simulate(*cloth);
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_LIGHTING);

    if (isLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    drawOtherStuff();
    if (isBall) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        cloth->drawBalls(showForce, size);
    } else {
        cloth->draw();
    }

    if (outputImage) {
        writeImage();
    }

    glFlush();
    glutSwapBuffers();
}

void screenMode() {
    if (isFullScreen) {
        glutFullScreen();
    } else {
        glutReshapeWindow(viewport.w, viewport.h);
    }
}

void myFrameMove(int whatever) {
    chooseCloth();
    screenMode();
    glutPostRedisplay();

    glutTimerFunc(FRAME_TIME, myFrameMove, 0);
}

void resetCloth(Cloth& c) {
    // Reset display booleans.
    //isLine = false;
    //isBall = false; showForce = false;
    isWind = false;

    // Reset movable ball if mode is 1.
    if (mode == 1) { ballCenter = glm::vec3(7.0f, -5.0f, 3.0f); ballRadius = 2.0f; }

    // Reset cloth state.
    c.reset();
}

// GLUT menu function.
void processMenuEvents(int option) {
    mode = option;
    chooseCloth();
    screenMode();
    resetCloth(*cloth);
    if (isFullScreen) {
        setCamera(fullScreen.w, fullScreen.h);
    } else {
        setCamera(viewport.w, viewport.h);
    }
}

void createGLUTMenus() {

    int menu;

    // create the menu and
    // tell glut that "processMenuEvents" will
    // handle the events
    menu = glutCreateMenu(processMenuEvents);

    //add entries to our menu
    glutAddMenuEntry("Hung Cloth", 0);
    glutAddMenuEntry("Hung Cloth and movable ball", 1);
    glutAddMenuEntry("Dropped cloth onto ball", 2);
    glutAddMenuEntry("Dropped cloth onto cube", 3);
    glutAddMenuEntry("Dropped cloth onto table", 4);
    glutAddMenuEntry("Dropped cloth onto stick", 5);
    glutAddMenuEntry("Dropped cloth onto floor", 6);

    // attach the menu to the right button
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// GLUT key call back.
void keyboard( unsigned char key, int x, int y ) {
    switch ( key ) {
    case 'q':    
        exit (0);
        break;
    case 'o':
        outputImage = !outputImage;
        if (outputImage) {
            cout << "Image output is on." << endl;
        } else {
            cout << "Image output is off." << endl;
        }
        break;
    case 'p':
        isPaused = !isPaused;
        break;
    case 'f':
        isFullScreen = !isFullScreen;
        break;
    case 'w':
        isWind = !isWind;
        break;
    case 'l':
        isLine = !isLine;
        break;
    case 'b':
        isBall = !isBall;
        break;
    case 'c':
        showForce = !showForce;
        break;
    case 'r':
        resetCloth(*cloth);
        break;
    case '0':
        processMenuEvents(0);
        break;
    case '1':
        processMenuEvents(1);
        break;
    case '2':
        processMenuEvents(2);
        break;
    case '3':
        processMenuEvents(3);
        break;
    case '4':
        processMenuEvents(4);
        break;
    case '5':
        processMenuEvents(5);
        break;
    case '6':
        processMenuEvents(6);
        break;
    default:
        break;
    }
}

void specialKey( int key, int x, int y ) {
    switch(key) {
    case GLUT_KEY_UP:
        ballCenter -= stepZ;
        break;
    case GLUT_KEY_DOWN:
        ballCenter += stepZ;
        break;
    case GLUT_KEY_LEFT:
        ballCenter -= stepX;
        break;
    case GLUT_KEY_RIGHT:
        ballCenter += stepX;
        break;
    default:
        break;
    }
}

// Manual.
void printManual() {
    cout << "-0: (default) Hung cloth." << endl;
    cout << "-1: Hung cloth with movable ball (use arrow keys to control)." << endl;
    cout << "-2: Dropped cloth with air resistance over a ball." << endl;
    cout << "-3: Dropped cloth with air resistance over a cube." << endl;
    cout << "-4: Dropped cloth with air resistance over a table (bit cube)." << endl;
    cout << "-5: Dropped cloth with air resistance over a stick (long cube)." << endl;
    cout << "-6: Dropped cloth with air resistance on the floor." << endl;
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "YOU COULD ALSO USE THE RIGHT CLICK MENU TO SELECT SCENE WHILE IN THE PROGRAM." << endl;
    cout << "-----------------------------------------------------------------------------" << endl;
    cout << "Supported operations:" << endl;
    cout << "0 ~ 6: switch between different scene." << endl;
    cout << "p: pause." << endl;
    cout << "o: turn of/off image frame output." << endl;
    cout << "w: turn on/off wind." << endl;
    cout << "l: turn on/off line mode." << endl;
    cout << "b: turn on/off ball mode." << endl;
    cout << "c: turn on/off shade ball with force (ball mode must be on)." << endl;
    cout << "r: reset the state of the current scene." << endl;
    cout << "f: turn on/off full screen mode." << endl;
    cout << "q: quit program." << endl;
}

int main(int argc, char *argv[]) {

    if (argc == 1) {
        mode = 0;
        cout << "Default mode entered, you could also try:" << endl;
        printManual();
    } else if (argc == 2) {
        if (!strcmp(argv[1], "-0")) {
            mode = 0;
        } else if (!strcmp(argv[1], "-1")) {
            mode = 1;
        } else if (!strcmp(argv[1], "-2")) {
            mode = 2;
        } else if (!strcmp(argv[1], "-3")) {
            mode = 3;
        } else if (!strcmp(argv[1], "-4")) {
            mode = 4;
        } else if (!strcmp(argv[1], "-5")) {
            mode = 5;
        } else if (!strcmp(argv[1], "-6")) {
            mode = 6;
        } else {
            cout << "Mode not supported. Currently supports:" << endl;
            printManual();
            exit(1);
        }
    } else {
        cout << "Irregular command line inputs." << endl;
        exit(1);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH ); 

    viewport.w = 800; viewport.h = 600;
    viewport.px = 500; viewport.py = 100;

    glutInitWindowSize(viewport.w, viewport.h);
    glutInitWindowPosition(viewport.px, viewport.py);
    glutCreateWindow("Cloth Simulation");
    initScene();

    glutDisplayFunc(myDisplay);
    glutReshapeFunc(myReshape);

    glutTimerFunc(FRAME_TIME, myFrameMove, 0);

    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    createGLUTMenus();

    glutMainLoop();

    return 0;
}
