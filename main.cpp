#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include "color.h"

using namespace std;

GLUquadricObj *quad = gluNewQuadric();

void initRendering()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);
}

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void menara()
{
    //menara polos
    glPushMatrix();
        glColor3f(White);
        glTranslated(0,-2,-6);
        glRotated(65, -1.0, 0.0, 0.0);
        gluCylinder(quad, 0.5, 0.33, 4, 50, 50);
    glPopMatrix();

    //lingkaran di menara bawah
    glPushMatrix();
        glColor3f(Silver);
        glTranslated(0,-1.2,-5.6);
        glRotated(85, -1.0, 0.0, 0.0);
        glutSolidTorus(0.08,0.5,50,50);
    glPopMatrix();

    //lingkaran di menara tengah
    glPushMatrix();
        glColor3f(Silver);
        glTranslated(0,0.3,-4.9);
        glRotated(80, -1.0, 0.0, 0.0);
        glutSolidTorus(0.08,0.4,50,50);
    glPopMatrix();

    //lingkaran di menara atas
    glPushMatrix();
        glColor3f(Silver);
        glTranslated(0,1.7,-4.4);
        glRotated(67, -1.0, 0.0, 0.0);
        glutSolidTorus(0.05,0.4,50,50);
    glPopMatrix();
}

void pohon()
{
    //Batang Cemara
    glPushMatrix();
glColor3f(0.8f, 0.4f, 0.1f);
    glScaled(1.1,7,1);
    glutSolidCube(0.5f);
    glPopMatrix();

    //Daun Bawah
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 1.5f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
glScaled(2,2,3);
glutSolidCone(1.6,1,20,30);
glPopMatrix();

    //Daun Tengah
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 3.0f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
glScaled(2,2,3);
glutSolidCone(1.3,1,20,30);
glPopMatrix();

//Daun Atas
    glPushMatrix();
    glColor3f(0.5f, 0.7f, 0.1f);
    glTranslatef(0.0f, 4.5f, 0.0f);
    glRotatef(230, 1.5, 2, 2);
glScaled(2,2,3);
glutSolidCone(1.0,1,20,30);
glPopMatrix();

}

void reshape(int w, int h)
{
    glViewport(0,0,(GLsizei) w,(GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
    glMatrixMode(GL_MODELVIEW);
}

void display(void)
{
    glClearStencil(0);
	glClearDepth(1.0f);
	glClearColor(Black, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity();
    menara();
    glutSwapBuffers();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutInitWindowPosition(100,100);
    glutCreateWindow("Menara");
    initRendering();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}
