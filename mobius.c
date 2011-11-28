#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "bmp.h"
#if defined(__APPLE__) || defined(MACOSX)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define MOB_N 100
#define MOB_M 10

#define INDICES_PER_STRIP (2*MOB_M)
#define NUM_STRIPS MOB_N
#define NUM_STRIP_INDICES (NUM_STRIPS*INDICES_PER_STRIP)
GLushort mobStrips[NUM_STRIP_INDICES];
#define VERTEX_INDEX(i,j) ((i)*MOB_M + (j))

GLuint texName;
BmpImage image;

GLdouble mob[MOB_N][MOB_M][3];
GLdouble texCoords[MOB_N][MOB_M][2];
GLdouble texCoordsOpp[MOB_N][MOB_M][2];
GLdouble mobNormals[MOB_N][MOB_M][3];
GLdouble mobNormalsOpp[MOB_N][MOB_M][3];
static int force_redraw = 1;
#define PLACE 50 // this zooms the camera out, sets the eye
GLdouble eye[3] = {0.0, 0.0, PLACE};
GLdouble lookat[3] = {0.0, 0.0, 0.0};
GLdouble up[3] = {0.0, 0.0, 1.0};
GLdouble hither = 0.1, yon = 100.0;

GLfloat theta = 0.0;

GLfloat lmodel_ambient[4]    = {0.1, 0.1, 0.1, 1.0};
GLfloat light0_position[4]   = {0.0, 0.0, PLACE, 0.0};
GLfloat light1_position[4]   = {0.0, 0.0, -PLACE, 0.0};
GLfloat light0_ambient[4]    = {0.3, 0.3, 0.3, 1.0};
GLfloat light0_diffuse[4]    = {1.0, 1.0, 1.0, 1.0};
GLfloat light0_specular[4]   = {1.0, 1.0, 1.0, 1.0};

GLdouble eyeRadius = PLACE, eyePhi = M_PI/4.0, eyeTheta = M_PI/4.0;

GLfloat ambient[4] = {0.24725, 0.1995, 0.0745, 1.0};
GLfloat diffuse[4] = {0.75164, 0.60648, 0.22648, 1.0};
GLfloat specular[4] = {0.628281, 0.555802, 0.366025, 1.0};
GLfloat shininess = 51.2;

bool mouseRotate = false;
int mousex, mousey;
const double epsilon = 0.0000001;

void setView();

void normalize(GLdouble A[3]){
    GLdouble mag = sqrt(A[0]*A[0]+A[1]*A[1]+A[2]*A[2]);
    A[0] /= mag;
    A[1] /= mag;
    A[2] /= mag;
}

void createMeshStrips(GLdouble width, GLdouble R){
    GLdouble ds = 2.0*width / (MOB_M-1);
    GLdouble dt = (2.0*M_PI) / (MOB_N-1);
    GLdouble t = 0;
    for(int i = 0; i < MOB_N; i++, t+=dt){
        GLdouble s = -width;
        for(int j = 0; j < MOB_M; j++, s+=ds){
            GLdouble x = (R + s * cos(.5*t))*cos(t);
            GLdouble y = (R + s * cos(.5*t))*sin(t);
            GLdouble z = s * sin(.5*t);
            mob[i][j][0] = x;
            mob[i][j][1] = y;
            mob[i][j][2] = z;

            // http://www.calpoly.edu/~dhartig/Pages/304/MapleTable/LabDemos/MobiusStrip.pdf
            // used Maple to find functions for mobius strip with radius R, instead of 1
            mobNormals[i][j][0] = -1.0/4.0*s*sin(2.0*t) + .5*sin(t)*s - .5*R*sin(3.0/2.0*t) + .5*R*sin(.5*t);
            mobNormals[i][j][1] = -.5*cos(t)*s+.25*s*cos(2.0*t)-.25*s-.5*R*cos(.5*t)+.5*R*cos(3.0/2.0*t);
            mobNormals[i][j][2] = (.5*s+.5*cos(t)*s+cos(.5*t)*R);
            normalize(mobNormals[i][j]);
            mobNormalsOpp[i][j][0] = -mobNormals[i][j][0];
            mobNormalsOpp[i][j][1] = -mobNormals[i][j][1];
            mobNormalsOpp[i][j][2] = -mobNormals[i][j][2];
        }
    }

    // texture coords
    for(int i = 0; i < MOB_N; i++, t+=dt){
        GLdouble s = -width;
        for(int j = 0; j < MOB_M; j++, s+=ds){
            // the magic values that make a 100x100 image look good
            GLdouble magic_x = 5;
            GLdouble magic_y = 2;
            GLdouble x = t/(2.0*M_PI/magic_x);
            GLdouble y = s/(2.0*width/magic_y);
            texCoords[i][j][0] = x;
            texCoords[i][j][1] = y;
            texCoordsOpp[i][j][0] = x;
            texCoordsOpp[i][j][1] = -y;
        }
    }
}

void createMeshStripIndices(void) {
    int n = 0;
    int i,j;
    for (i = 0; i < MOB_N; i++) {
        for (j = 0; j < MOB_M; j++) {
            mobStrips[n++] = VERTEX_INDEX((i+1)%MOB_N,j);
            mobStrips[n++] = VERTEX_INDEX(i,j);
        }
    }
}

void drawMesh(int opp){
    static GLuint vertBuffer;
    static GLuint indexBuffer;
    static GLuint normBuffer;
    static GLuint normBufferOpp;
    static GLuint texBuffer;
    static GLuint texBufferOpp;
    static int is_first = 1;

    if(is_first){
        glEnableClientState(GL_NORMAL_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        glGenBuffers(1, &vertBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
        glVertexPointer(3, GL_DOUBLE, 0, (GLvoid*) 0);

        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer); 

        glGenBuffers(1, &normBuffer);
        glGenBuffers(1, &normBufferOpp);
        glBindBuffer(GL_ARRAY_BUFFER, normBuffer); 

        glGenBuffers(1, &texBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, texBuffer); 

        glGenBuffers(1, &texBufferOpp);
        glBindBuffer(GL_ARRAY_BUFFER, texBufferOpp); 

        is_first = 0;
    }

    if(force_redraw){
        // indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mobStrips), mobStrips, GL_STATIC_DRAW);

        // normals
        glBindBuffer(GL_ARRAY_BUFFER, normBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mobNormals), mobNormals, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, normBufferOpp);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mobNormals), mobNormalsOpp, GL_STATIC_DRAW);

        // texture
        glBindBuffer(GL_ARRAY_BUFFER, texBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, texBufferOpp);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoordsOpp), texCoordsOpp, GL_STATIC_DRAW);

        // verts
        glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mob), mob, GL_STATIC_DRAW);
    }

    if(opp){
        glBindBuffer(GL_ARRAY_BUFFER, normBuffer); 
        glNormalPointer(GL_DOUBLE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, texBufferOpp); 
        glTexCoordPointer(2, GL_DOUBLE, 0, (GLvoid*) 0);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, normBufferOpp); 
        glNormalPointer(GL_DOUBLE, 0, (GLvoid*)0);

        glBindBuffer(GL_ARRAY_BUFFER, texBuffer); 
        glTexCoordPointer(2, GL_DOUBLE, 0, (GLvoid*) 0);
    }

    int i = 0;
    char *offset = 0;
    // I do not draw the last strip, since it over laps the first
    for (; i < NUM_STRIPS-1; i++, offset += INDICES_PER_STRIP*sizeof(GLushort)){
        glDrawElements(GL_TRIANGLE_STRIP, INDICES_PER_STRIP, GL_UNSIGNED_SHORT, (GLvoid*) offset);
    }
}

void reshape(int w, int h) {
    glViewport( 0, 0, w, h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective(40, ((double)(w)/(double)(h)), hither, yon );
}

void getTexture(){
    //glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGenTextures(1, &texName);
    bmpGetImage("tile.bmp", &image, 0);

    glBindTexture(GL_TEXTURE_2D, texName);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.W, image.H, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba);
}

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if(force_redraw){
        createMeshStrips(4, 14);
        createMeshStripIndices();
    }

    // Draw front
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);  
    drawMesh(0);
    // Draw back
    glCullFace(GL_FRONT);  
    drawMesh(1);
    setView();
    glutSwapBuffers();
    force_redraw = 0;
}

void init() {
    //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glEnable(GL_LIGHT0);

    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shininess);

    glEnable(GL_TEXTURE_2D);
    getTexture();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindTexture(GL_TEXTURE_2D, texName);
    static GLfloat labelColor[4] = {0.5, 0.5, 0.5, 1.0};
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, labelColor); 

    setView();

    glEnable(GL_DEPTH_TEST);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void sphericalToCartesian(double r, double theta, double phi, double *x, double *y, double *z) {
    double sin_phi = sin(phi);
    *x = r*cos(theta)*sin_phi;
    *y = r*sin(theta)*sin_phi;
    *z = r*cos(phi);
}

void setView() {
    sphericalToCartesian(eyeRadius, eyeTheta, eyePhi, &eye[0], &eye[1], &eye[2]);
    eye[0] += lookat[0]; eye[1] += lookat[1]; eye[2] += lookat[2];
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    gluLookAt(eye[0], eye[1], eye[2], lookat[0], lookat[1], lookat[2], up[0], up[1], up[2]);
}

void mouse(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        if (button == GLUT_LEFT_BUTTON) {
            mouseRotate = true;
            mousex = x;
            mousey = y;
        }
    } else if (state == GLUT_UP) {
        mouseRotate = false;
    }
}

void mouseMotion(int x, int y) {
    if (mouseRotate) {
        const double radiansPerPixel = M_PI/(2*90.0);
        int dx = x - mousex, dy = y - mousey;
        eyeTheta -= dx*radiansPerPixel;
        eyePhi -= dy*radiansPerPixel;
        if (eyePhi >= M_PI)
            eyePhi = M_PI - epsilon;
        else if (eyePhi <= 0.0)
            eyePhi = epsilon;
        setView();
        mousex = x;
        mousey = y;
        glutPostRedisplay();
    }
}

GLfloat seconds = 0;
static void idle(void) {
    static GLfloat next_stop = 0;
    seconds = glutGet(GLUT_ELAPSED_TIME)/1000.0;
    if(seconds > next_stop){
        next_stop = seconds + .025;
        glMatrixMode(GL_TEXTURE);
        glTranslatef(.05, 0, 0);
        glMatrixMode(GL_MODELVIEW);
        theta += .5;
        //glRotatef(theta, 1, .5, 0);
        glutPostRedisplay();
    }
}

int main(int argc, char **argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH); /* double buffering */
    glEnable(GL_DEPTH_TEST);
    glutInitWindowSize(800, 800);
    glutInitWindowPosition(10, 10);
    glutCreateWindow("Tilt-A-Whirl");
    glutMouseFunc(mouse);
    glutIdleFunc(idle);
    //glutIdleFunc(idle);
    //glutKeyboardFunc(keyboard);
    glutMotionFunc(mouseMotion);
    glutReshapeFunc(reshape);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glutDisplayFunc(display);
    init();
    glutMainLoop();
    return 0;
}
