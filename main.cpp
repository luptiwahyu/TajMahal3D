#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#include "color.h"
#endif

using namespace std;

static GLfloat spin, spin2 = 0.0;
float angle = 0;

float lastx, lasty;
GLint stencilBits;
static int viewx = 200;
static int viewy = 20;
static int viewz = -1;

float rot = 0;

float xk, xl, zl;
GLUquadricObj *quad = gluNewQuadric();
GLuint texture[2];

//train 2D
//class untuk terain 2D
class Terrain {
private:
	int w; //Width
	int l; //Length
	float** hs; //Heights
	Vec3f** normals;
	bool computedNormals; //Whether normals is up-to-date
public:
	Terrain(int w2, int l2) {
		w = w2;
		l = l2;

		hs = new float*[l];
		for (int i = 0; i < l; i++) {
			hs[i] = new float[w];
		}

		normals = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals[i] = new Vec3f[w];
		}

		computedNormals = false;
	}

	~Terrain() {
		for (int i = 0; i < l; i++) {
			delete[] hs[i];
		}
		delete[] hs;

		for (int i = 0; i < l; i++) {
			delete[] normals[i];
		}
		delete[] normals;
	}

	int width() {
		return w;
	}

	int length() {
		return l;
	}

	//Sets the height at (x, z) to y
	void setHeight(int x, int z, float y) {
		hs[z][x] = y;
		computedNormals = false;
	}

	//Returns the height at (x, z)
	float getHeight(int x, int z) {
		return hs[z][x];
	}

	//Computes the normals, if they haven't been computed yet
	void computeNormals() {
		if (computedNormals) {
			return;
		}

		//Compute the rough version of the normals
		Vec3f** normals2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			normals2[i] = new Vec3f[w];
		}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);

				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
				}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
				}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
				}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
				}

				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();
				}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();
				}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();
				}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();
				}

				normals2[z][x] = sum;
			}
		}

		//Smooth out the normals
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = normals2[z][x];

				if (x > 0) {
					sum += normals2[z][x - 1] * FALLOUT_RATIO;
				}
				if (x < w - 1) {
					sum += normals2[z][x + 1] * FALLOUT_RATIO;
				}
				if (z > 0) {
					sum += normals2[z - 1][x] * FALLOUT_RATIO;
				}
				if (z < l - 1) {
					sum += normals2[z + 1][x] * FALLOUT_RATIO;
				}

				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);
				}
				normals[z][x] = sum;
			}
		}

		for (int i = 0; i < l; i++) {
			delete[] normals2[i];
		}
		delete[] normals2;

		computedNormals = true;
	}

	//Returns the normal at (x, z)
	Vec3f getNormal(int x, int z) {
		if (!computedNormals) {
			computeNormals();
		}
		return normals[z][x];
	}
};
//end class


void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
}

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
//load terain di procedure inisialisasi
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3 * (y
					* image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}

	delete image;
	t->computeNormals();
	return t;
}

float _angle = 60.0f;
//buat tipe data terain
Terrain* _terrain;
Terrain* _terrainTanah;
Terrain* _terrainKolam;

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

void cleanup() {
	delete _terrain;
	delete _terrainKolam;
}

//untuk di display
void drawSceneTanah(Terrain *terrain, GLfloat r, GLfloat g, GLfloat b) {
	//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	/*
	 glMatrixMode(GL_MODELVIEW);
	 glLoadIdentity();
	 glTranslatef(0.0f, 0.0f, -10.0f);
	 glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
	 glRotatef(-_angle, 0.0f, 1.0f, 0.0f);

	 GLfloat ambientColor[] = {0.4f, 0.4f, 0.4f, 1.0f};
	 glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

	 GLfloat lightColor0[] = {0.6f, 0.6f, 0.6f, 1.0f};
	 GLfloat lightPos0[] = {-0.5f, 0.8f, 0.1f, 0.0f};
	 glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
	 glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);
	 */
	float scale = 400.0f / max(terrain->width() - 1, terrain->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (terrain->width() - 1) / 2, 0.0f,
			-(float) (terrain->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);
		for (int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}

}

unsigned int LoadTextureFromBmpFile(char *filename);

/* segidelapan bagian tengah */
void kubusbesarinti()
{
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,1.5,1.0);
        glScalef(1.32,1.0,1.0);
        glutSolidCube(2);
    glPopMatrix();
}

void kubuskecilinti()
{
    glPushMatrix();
        glColor3f(White);
        glTranslatef(0.0,1.5,2.05);
        glScalef(1.0,1.0,0.25);
        glutSolidCube(2);
    glPopMatrix();
}

void bagianinti()
{
    glPushMatrix();
        glTranslatef(0.0,0.0,0.0);
        kubusbesarinti();
    glPopMatrix();

    glPushMatrix();
        kubuskecilinti();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0.0,0.0,-2.12);
        kubuskecilinti();
    glPopMatrix();
}

void sudutbangunaninti()
{
    glPushMatrix();
        glColor3f(White);
        glTranslated(1.88,2.5,2.49);
        glRotated(90,1.0,0.0,0.0);
        glRotated(255,0.0,0.0,1.0);
        gluCylinder(quad,0.25,0.25,2,3,3);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslatef(1.88,2.5,2.49);
        glRotated(270,1.0,0.0,0.0);
        glRotated(45,0.0,0.0,1.0);
        gluDisk(quad,0,0.25,3,1);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslatef(1.88,0.5,2.49);
        glRotated(90,1.0,0.0,0.0);
        glRotated(255,0.0,0.0,1.0);
        gluDisk(quad,0,0.25,3,1);
    glPopMatrix();
}

void sudutbangunanintifix()
{
    glPushMatrix();
        glTranslatef(-0.805,0.0,-0.43);
        glScalef(1.0,1.0,1.0);
        sudutbangunaninti();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-1.42,0.0,1.81);
        glScalef(1.0,1.0,1.0);
        glRotatef(90,0.0,1.0,0.0);
        sudutbangunaninti();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-4.17,0.0,0.305);
        glScalef(1.0,1.0,1.0);
        glRotatef(60,0.0,1.0,0.0);
        sudutbangunaninti();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(1.415,0.0,0.185);
        glScalef(1.0,1.0,1.0);
        glRotatef(270,0.0,1.0,0.0);
        sudutbangunaninti();
    glPopMatrix();
}

// bangunan tengah inti segidelapan
void bangunandasarintifix()
{
    glPushMatrix();
        glTranslatef(0.0,-1.25,0.0);
        glScalef(2.4,1.18,2.4);
        bagianinti();
        sudutbangunanintifix();
    glPopMatrix();
}

/* pintu */
void circle()
{
    //filled circle
    float x1,y1,x2,y2;
    float angle;
    double radius=0.1;

    x1=0.5, y1=0.6;

    glBegin(GL_TRIANGLE_FAN);
        glColor3f(Black);
        glVertex2f(x1,y1);
        for (angle=1.0f;angle<361.0f;angle+=0.2)
        {
            x2 = x1+sin(angle)*radius;
            y2 = y1+cos(angle)*radius;
            glVertex2f(x2,y2);
        }
    glEnd();
}

void square()
{
    glBegin(GL_POLYGON);
        glColor3f(Black);
        glVertex2f(0.2,0.8);
        glColor3f(Black);
        glVertex2f(-0.2,0.8);
        glColor3f(Black);
        glVertex2f(-0.2,0.2);
        glColor3f(Black);
        glVertex2f(0.2,0.2);
    glEnd();
}

void pintumasuk()
{
    // pintu masuk luar
    glPushMatrix();
        glTranslated(-2.0,-0.61,0.15);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.025);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.025);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // pintu masuk belakang
    glPushMatrix();
        glTranslated(-2.0,-0.61,-0.15);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();
}

void pintubesar()
{
    // kubus luar
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.8,1.0);
        glRotated(0.0,-1.0,0.0,0.0);
        glScaled(1.37,1.5,0.15);
        glutSolidCube(1.95);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0.0,-0.1,0.0);
        glScalef(1.5,1.2,1.0);
        pintumasuk();
    glPopMatrix();

    // tiang kanan
    glPushMatrix();
        glPushMatrix();
            glColor3f(White);
            glTranslated(1.35,-0.65,1.18);
            glRotated(90,-1.0,0.0,0.0);
            gluCylinder(quad,0.05,0.05,3.4,50,50);
        glPopMatrix();

        glPushMatrix();
            glColor3f(White);
            glTranslated(1.35,2.75,1.18);
            glRotated(90,-1.0,0.0,0.0);
            glutSolidSphere(0.06,50,50);
        glPopMatrix();
    glPopMatrix();

    // tiang kiri
    glPushMatrix();
        glPushMatrix();
            glColor3f(White);
            glTranslated(-1.35,-0.65,1.18);
            glRotated(90,-1.0,0.0,0.0);
            gluCylinder(quad,0.05,0.05,3.4,50,50);
        glPopMatrix();

        glPushMatrix();
            glColor3f(White);
            glTranslated(-1.35,2.75,1.18);
            glRotated(90,-1.0,0.0,0.0);
            glutSolidSphere(0.06,50,50);
        glPopMatrix();
    glPopMatrix();
}

void tiang(){
      // tiang kiri
    glPushMatrix();
        glPushMatrix();
            glColor3f(White);
            glTranslated(-1.35,-0.65,1.18);
            glRotated(90,-1.0,0.0,0.0);
            gluCylinder(quad,0.05,0.05,3.4,50,50);
        glPopMatrix();

        glPushMatrix();
            glColor3f(White);
            glTranslated(-1.35,2.75,1.18);
            glRotated(90,-1.0,0.0,0.0);
            glutSolidSphere(0.06,50,50);
        glPopMatrix();
    glPopMatrix();
}

void pintukecil()
{
    // kubus luar
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.8,1.0);
        glRotated(0.0,-1.0,0.0,0.0);
        glScaled(1.1,1.3,0.15);
        glutSolidCube(1.95);
    glPopMatrix();

    // pintu masuk luar
    glPushMatrix();
        glTranslated(-2.0,-0.61,0.15);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.025);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();
        glPushMatrix();
            glTranslated(2.0,-0.3,1.025);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // pintu masuk belakang
    glPushMatrix();
        glTranslated(-2.0,-0.61,-0.15);
        glPushMatrix();
            glTranslated(-0.85,-2.3,0.975);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,0.975);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();
}

void pintukecilfix()
{
    // pintu kecil
	glPushMatrix();
	glTranslatef(1.9,-0.43,0.0);
        glPushMatrix();
            glScalef(0.5,0.5,1.0);
            pintukecil();
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.0,1.1,0.0);
            glScalef(0.5,0.5,1.0);
            pintukecil();
        glPopMatrix();
    glPopMatrix();
}

void pintubesarkecil()
{
    pintukecilfix();
    // 2 pintu kiri
	glPushMatrix();
	glTranslatef(-1.9,-0.43,0.0);
        glPushMatrix();
            glScalef(0.5,0.5,1.0);
            pintukecil();
        glPopMatrix();
        glPushMatrix();
            glTranslatef(0.0,1.1,0.0);
            glScalef(0.5,0.5,1.0);
            pintukecil();
        glPopMatrix();
    glPopMatrix();

    //tiang kiri pendek
    glPushMatrix();
        glTranslatef(-1.1,-0.1,0.0);
        glScalef(1.0,0.85,1.0);
        tiang();
    glPopMatrix();

    //tiang kanan pendek
     glPushMatrix();
        glTranslatef(3.8,-0.1,0.0);
        glScalef(1.0,0.85,1.0);
        tiang();
    glPopMatrix();
    pintubesar();
}

// pintu besar kecil
void pintubesarkecilfix()
{
    glPushMatrix();
        glTranslatef(0.0,0.0,4.5);
        pintubesarkecil();
    glPopMatrix();
}

/* kubah besar */
void kubahbesar()
{
    // menara atas
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.0,1.0);
        glRotated(90,1.0,0.0,0.0);
        gluCylinder(quad,0.37,0.37,0.35,50,50);
    glPopMatrix();

    // kubah
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.19,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidSphere(0.42,50,50);
    glPopMatrix();
}

void tiangkubah()
{
    // tiang silinder
    glColor3f(GoldPole);
    glPushMatrix();
        glTranslated(0.0,0.98,1.0);
        glScaled(1.0,8.0,1.0);
        glRotated(90,1.0,0.0,0.0);
        gluCylinder(quad,0.012,0.012,0.035,50,50);
    glPopMatrix();

    // bulatan atas di tiang
    glPushMatrix();
        //glColor3f(225.0f/255,225.0f/255,0);
        glTranslated(0.0,0.92,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidSphere(0.0275,50,50);
    glPopMatrix();

    // bulatan tengah di tiang
    glPushMatrix();
        //glColor3f(225.0f/255,225.0f/255,0);
        glTranslated(0.0,0.83,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidSphere(0.035,50,50);
    glPopMatrix();

    // bulatan bawah di tiang
    glPushMatrix();
        //glColor3f(225.0f/255,225.0f/255,0);
        glTranslated(0.0,0.74,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidSphere(0.0275,50,50);
    glPopMatrix();

    // ujung tiang
    glPushMatrix();
        //glColor3f(225.0f/255,225.0f/255,0);
        glTranslated(0.0,0.98,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidCone(0.012,0.05,50,50);
    glPopMatrix();
}

void kerucutatas()
{
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.575,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidCone(0.18,0.15,20,20);
    glPopMatrix();
}

// kubah besar fix
void kubahbesarfix()
{
    glPushMatrix();
        glColor3f(Black);
        glTranslated(0.0,1.2,-1.0);
        glScaled(3.5,3.2,3.5);
        tiangkubah();
        kerucutatas();
        kubahbesar();
    glPopMatrix();
}

/* kubah kecil */
void kubahkecilfix()
{
    // menara atas
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.0,1.0);
        glRotated(90,1.0,0.0,0.0);
        gluCylinder(quad,0.4,0.4,0.7,8,10);
    glPopMatrix();

    // jendela 1
    glPushMatrix();
        glRotated(-22,0.0,1.0,0.0);
        glTranslated(0.07,-0.74,1.3);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            //glColor3f(White);
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 2
    glPushMatrix();
        glRotated(22,0.0,1.0,0.0);
        glTranslated(-0.68,-0.74,0.55);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 3
    glPushMatrix();
        glRotated(-22,0.0,-1.2,0.0);
        glTranslated(-0.67,-0.74,1.3);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 4
    glPushMatrix();
        glRotated(22,0.0,-1.2,0.0);
        glTranslated(0.07,-0.74,0.55);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 5
    glPushMatrix();
        glRotated(68,0.0,1.0,0.0);
        glTranslated(-1.23,-0.74,0.75);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 6
    glPushMatrix();
        glRotated(-68,0.0,1.0,0.0);
        glTranslated(0.63,-0.74,0.75);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 7
    glPushMatrix();
        glRotated(-112,0.0,1.0,0.0);
        glTranslated(0.63,-0.74,0.002);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // jendela 8
    glPushMatrix();
        glRotated(112,0.0,1.0,0.0);
        glTranslated(-1.22,-0.74,0.002);
        glScaled(0.15,0.25,0.0);
        glPushMatrix();
            glTranslated(-0.85,-2.3,1.0);
            glScaled(5.7,6.2,1.0);
            circle();
        glPopMatrix();

        glPushMatrix();
            glTranslated(2.0,-0.3,1.0);
            glScaled(3.0,2.2,1.0);
            square();
        glPopMatrix();
    glPopMatrix();

    // kubah
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.15,1.0);
        glRotated(90,-1.0,0.0,0.0);
        glutSolidSphere(0.42,50,50);
    glPopMatrix();

    // ring
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.0,1.0);
        glScaled(1.0,0.15,1.0);
        glRotated(90,1.0,0.0,0.0);
        glutSolidSphere(0.6,8,10);
    glPopMatrix();

    // tiang
    glPushMatrix();
        glTranslatef(0.0,-0.14,0.0);
        tiangkubah();
    glPopMatrix();
}

/* lantai */
void lantai()
{
    glPushMatrix();
            glScalef(5.0,-0.25,5.0);
            glutSolidCube(2);
    glPopMatrix();
}

void tanggadepan()
{
    //tangga depan dan belakang
    glPushMatrix();
            glScalef(1.0,-0.25,0.5);
            glutSolidCube(2);
    glPopMatrix();
    }

void tanggasamping()
{
    //tangga kanan dan kiri
    glPushMatrix();
            glScalef(0.7,-0.25,1.0);
            glutSolidCube(2);
    glPopMatrix();
}

void lantaibulat()
{
    glColor3f(White);
    glPushMatrix();
        glTranslated(0.0,-1.0,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.8,0.8,0.5,8,1);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0.0,-0.5,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0.0,0.8,8,1);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0.0,-1.0,1.0);
        glRotated(270,-1.0,0.0,0.0);
        gluDisk(quad,0.0,0.8,8,1);
    glPopMatrix();
}

void lantaifix()
{
    glColor3f(White);
    lantai();
    //tangga depan
    glPushMatrix();
        glTranslatef(0.0,0.0,5.5);
        tanggadepan();
    glPopMatrix();

    //tangga belakang
    glPushMatrix();
        glTranslatef(0.0,0.0,-5.5);
        tanggadepan();
    glPopMatrix();

    //tangga kanan
	glPushMatrix();
        glTranslatef(5.4,0.0,0.0);
       // glRotated(45,1.0,1.0,1.0);
        tanggasamping();
	glPopMatrix();

    //tangga kiri
    glPushMatrix();
        glTranslatef(-5.4,0.0,0.0);
        tanggasamping();
    glPopMatrix();
}

void menara()
{
    // menara polos
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,-1.0,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.55,0.4,4.5,50,50);
    glPopMatrix();

    // ring bawah
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.05,1.0);
        glScaled(1.0,0.15,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.65,0.65,0.8,50,50);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.17,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0,0.65,50,1);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,0.05,1.0);
        glRotated(270,-1.0,0.0,0.0);
        gluDisk(quad,0,0.65,50,1);
    glPopMatrix();

    // ring tengah
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,1.75,1.0);
        glScaled(1.0,0.15,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.61,0.61,0.8,50,50);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,1.87,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0,0.61,50,1);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,1.75,1.0);
        glRotated(270,-1.0,0.0,0.0);
        gluDisk(quad,0,0.61,50,1);
    glPopMatrix();

    // ring atas
    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,3.45,1.0);
        glScaled(1.0,0.15,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluCylinder(quad,0.57,0.57,0.8,50,50);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,3.57,1.0);
        glRotated(90,-1.0,0.0,0.0);
        gluDisk(quad,0,0.57,50,1);
    glPopMatrix();

    glPushMatrix();
        glColor3f(White);
        glTranslated(0.0,3.45,1.0);
        glRotated(270,-1.0,0.0,0.0);
        gluDisk(quad,0,0.57,50,1);
    glPopMatrix();
}

void menarafix()
{
    glPushMatrix();
        glTranslated(0.0,-1.0,0.0);
        menara();
        glTranslated(0.0,4.2,0.0);
        kubahkecilfix();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0.0,-1.45,0.0);
        glScalef(1.0,1.1,1.0);
        lantaibulat();
    glPopMatrix();
}

void tajmahal()
{
    bangunandasarintifix();
    pintubesarkecilfix();
    glPushMatrix();
        glRotatef(90,0.0,1.0,0.0);
        glTranslatef(-2.4,0.0,-2.4);
        pintubesarkecilfix();
    glPopMatrix();

    glPushMatrix();
        glRotatef(180,0.0,1.0,0.0);
        glTranslatef(0.0,0.0,-4.75);
        pintubesarkecilfix();
    glPopMatrix();

    glPushMatrix();
        glRotatef(270,0.0,1.0,0.0);
        glTranslatef(2.4,0.0,-2.4);
        pintubesarkecilfix();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(4.8,0.0,4.5);
        glRotatef(225,0.0,1.0,0.0);
        pintukecilfix();
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-0.70,0.0,-1.0);
        glRotatef(225,0.0,1.0,0.0);
        pintukecilfix();
    glPopMatrix();
    glPushMatrix();
        glTranslatef(2.1,0.0,-2.4);
        glRotatef(315,0.0,1.0,0.0);
        pintukecilfix();
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-3.35,0.0,3.08);
        glRotatef(315,0.0,1.0,0.0);
        pintukecilfix();
    glPopMatrix();

    // kubah besar
    glPushMatrix();
        glTranslatef(0.0,1.45,0.0);
        kubahbesarfix();
    glPopMatrix();

    for(xk=-2.0;xk<4.1;xk+=4.0)
    {
        glPushMatrix();
            glTranslated(xk,2.4,-0.9);
            glScaled(1.3,1.1,1.3);
            kubahkecilfix();
        glPopMatrix();

        glPushMatrix();
            glTranslated(xk,2.4,3.1);
            glScaled(1.3,1.1,1.3);
            kubahkecilfix();
        glPopMatrix();
    }

    glPushMatrix();
        glTranslatef(0.0,-1.0,2.3);
        glScalef(1.0,1.1,1.0);
        lantaifix();
    glPopMatrix();

    for(xl=-5.0;xl<6.0;xl+=10)
    {
        glPushMatrix();
            glTranslatef(xl,1.275,6.0);
            menarafix();
        glPopMatrix();
    }

    for(xl=-5.0;xl<6.0;xl+=10)
    {
        glPushMatrix();
            glTranslatef(xl,1.275,-3.5);
            menarafix();
        glPopMatrix();
    }
}

void pohon(){
    glPushMatrix();
        glColor3f(Green);
        glTranslatef(0,-1,0);
        glRotated(90,-1,0,0);
        gluCylinder(quad,0.5,0.5,2.0,50,50);
    glPopMatrix();

    glPushMatrix();
        glTranslated(0,1,0);
        gluSphere(quad,0.5,50,50);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0,-1,0);
        glRotated(270,-1,0,0);
        gluDisk(quad,0,0.5,50,1);
    glPopMatrix();

    glPushMatrix();
        glColor3f(Brown);
        glTranslatef(0,-1.5,0);
        glRotated(90,-1,0,0);
        gluCylinder(quad,0.2,0.2,2.0,50,50);

    glPopMatrix();
}

void awan(void)
{
    glPushMatrix();
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColor3ub(153, 223, 255);
        glutSolidSphere(10, 50, 50);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(10,0,1);
        glutSolidSphere(5, 50, 50);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-2,6,-2);
        glutSolidSphere(7, 50, 50);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(-10,-3,0);
        glutSolidSphere(7, 50, 50);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(6,-2,2);
        glutSolidSphere(7, 50, 50);
    glPopMatrix();
}

void dasar()
{
    glColor3f(White);
    glPushMatrix();
        glBegin(GL_POLYGON);
            glTexCoord2f(0.0, 0.0);
            glVertex3f(1, 10, 0.0);
            glTexCoord2f(1.0, 0.0);
            glVertex3f(10, 10, 0.0);
            glTexCoord2f(1.0, 1.0);
            glVertex3f(10, 20, 0.0);
            glTexCoord2f(0.0, 1.0);
            glVertex3f(1, 20, 0.0);
        glEnd();
    glPopMatrix();
}

struct Gambar {
	unsigned long sizeX;
	unsigned long sizeY;
	char *data;
};
typedef struct Gambar Gambar; //struktur data untuk

//mengambil gambar BMP
int GambarLoad(char *filename, Gambar *gambar) {
	FILE *file;
	unsigned long size; // ukuran gambar dalam bytes
	unsigned long i; // standard counter.
	unsigned short int plane; // number of planes in gambar

	unsigned short int bpp; // jumlah bits per pixel
	char temp; // temporary color storage for var warna sementara untuk memastikan filenya ada


	if ((file = fopen(filename, "rb")) == NULL) {
		printf("File Not Found : %s\n", filename);
		return 0;
	}
	// mencari file header bmp
	fseek(file, 18, SEEK_CUR);
	// read the width
	if ((i = fread(&gambar->sizeX, 4, 1, file)) != 1) {
		printf("Error reading width from %s.\n", filename);
		return 0;
	}
	//printf("Width of %s: %lu\n", filename, gambar->sizeX);
	// membaca nilai height
	if ((i = fread(&gambar->sizeY, 4, 1, file)) != 1) {
		printf("Error reading height from %s.\n", filename);
		return 0;
	}
	//printf("Height of %s: %lu\n", filename, gambar->sizeY);
	//menghitung ukuran gambar(asumsi 24 bits or 3 bytes per pixel).

	size = gambar->sizeX * gambar->sizeY * 3;
	// read the planes
	if ((fread(&plane, 2, 1, file)) != 1) {
		printf("Error reading planes from %s.\n", filename);
		return 0;
	}
	if (plane != 1) {
		printf("Planes from %s is not 1: %u\n", filename, plane);
		return 0;
	}
	// read the bitsperpixel
	if ((i = fread(&bpp, 2, 1, file)) != 1) {
		printf("Error reading bpp from %s.\n", filename);

		return 0;
	}
	if (bpp != 24) {
		printf("Bpp from %s is not 24: %u\n", filename, bpp);
		return 0;
	}
	// seek past the rest of the bitmap header.
	fseek(file, 24, SEEK_CUR);
	// read the data.
	gambar->data = (char *) malloc(size);
	if (gambar->data == NULL) {
		printf("Error allocating memory for color-corrected gambar data");
		return 0;
	}
	if ((i = fread(gambar->data, size, 1, file)) != 1) {
		printf("Error reading gambar data from %s.\n", filename);
		return 0;
	}
	for (i = 0; i < size; i += 3) { // membalikan semuan nilai warna (gbr - > rgb)
		temp = gambar->data[i];
		gambar->data[i] = gambar->data[i + 2];
		gambar->data[i + 2] = temp;
	}
	// we're done.
	return 1;
}

//mengambil tekstur
Gambar * loadTexture() {
	Gambar *gambar1;
	// alokasi memmory untuk tekstur
	gambar1 = (Gambar *) malloc(sizeof(Gambar));
	if (gambar1 == NULL) {
		printf("Error allocating space for gambar");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!GambarLoad("pohon.bmp", gambar1)) {
		exit(1);
	}
	return gambar1;
}

Gambar * loadTextureDua() {
	Gambar *gambar1;
	// alokasi memmory untuk tekstur
	gambar1 = (Gambar *) malloc(sizeof(Gambar));
	if (gambar1 == NULL) {
		printf("Error allocating space for gambar");
		exit(0);
	}
	//pic.bmp is a 64x64 picture
	if (!GambarLoad("water.bmp", gambar1)) {
		exit(1);
		exit(1);
	}
	return gambar1;
}

void display(void) {

	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);
	glClearColor(0.0, 0.6, 0.8, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear the buffers
	glLoadIdentity();
	gluLookAt(viewx, viewy, viewz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);

	glDisable( GL_CULL_FACE );

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrain, 0.3f, 0.9f, 0.0f);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainTanah, 205.0f/255, 120.0f/255, 63.0f/255);
	glPopMatrix();

	glPushMatrix();

	//glBindTexture(GL_TEXTURE_3D, texture[0]);
	drawSceneTanah(_terrainKolam, 0.0f, 0.2f, 0.5f);
	glPopMatrix();

	glPushMatrix();
        glTranslatef(-100.0,32.0,-35.0);
        glScalef(15.0,15.0,15.0);
        tajmahal();
    glPopMatrix();

    float xp;
    for(xp=4;xp<=50;xp+=5)
    {
        glPushMatrix();
            glEnable(GL_TEXTURE_2D);
            //glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
            //glEnable(GL_TEXTURE_GEN_T);
            glBindTexture(GL_TEXTURE_2D, texture[0]);
            gluQuadricTexture(quad,1);
            glScaled(4,4,4);
            glTranslated(xp,4.3,5);
            pohon();
            //glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
            //glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();

        glPushMatrix();
            glEnable(GL_TEXTURE_2D);
            //glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
            //glEnable(GL_TEXTURE_GEN_T);
            glBindTexture(GL_TEXTURE_2D, texture[0]);
            //gluQuadricTexture(quad,1);
            glScaled(4,4,4);
            glTranslated(xp,4.3,-5);
            pohon();
            //glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
            //glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    //awan
    glPushMatrix();
        glTranslatef(-250, 100, -230);
        glScalef(1.8, 1.0, 1.0);
        glRotated(65,1,3,1);
        awan();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-250, 100, -50);
        glScalef(0.8, 1.0, 1.0);
        glRotated(65,1,3,1);
        awan();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-250, 180, 50);
        glScalef(2.8, 2.0, 2.0);
        glRotated(80,0,3,0);
        awan();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-250, 120, 200);
        glScalef(1.5, 1.5, 1.5);
        glRotated(80,1,3,1);
        awan();
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-250, 120, -140);
        glScalef(2.8, 2.0, 2.0);
        glRotated(65,1,3,1);
        awan();
    glPopMatrix();

    float xw;
    for(xw=13;xw<=180;xw+=16)
    {
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        //glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
        //glEnable(GL_TEXTURE_GEN_T);
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glTranslatef(xw,12.8,-31.3);
        glRotated(90,1,0,0);
        glScaled(1.8,2.05,1.8);
        dasar();
        //glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
        //glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        //glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
        //glEnable(GL_TEXTURE_GEN_T);
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glTranslatef(182,12.8,-31.3);
        glRotated(90,1,0,0);
        glScaled(1.8,2.05,1.8);
        dasar();
        //glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
        //glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();

	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_CULL_FACE);

	_terrain = loadTerrain("heightmap.bmp", 20);
	_terrainTanah = loadTerrain("heightmapTanah.bmp", 20);
	_terrainKolam = loadTerrain("heightmapKolam.bmp", 20);

	Gambar *gambar1 = loadTexture();
	Gambar *gambar2 = loadTextureDua();

	if (gambar1 == NULL) {
		printf("Gambar was not returned from loadTexture\n");
		exit(0);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Generate texture/ membuat texture
	glGenTextures(2, texture);

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//menyesuaikan ukuran textur ketika gambar lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //
	//menyesuaikan ukuran textur ketika gambar lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, gambar1->sizeX, gambar1->sizeY, 0, GL_RGB,
			GL_UNSIGNED_BYTE, gambar1->data);

	//tekstur air

	//binding texture untuk membuat texture 2D
	glBindTexture(GL_TEXTURE_2D, texture[2]);

	//menyesuaikan ukuran textur ketika gambar lebih besar dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //
	//menyesuaikan ukuran textur ketika gambar lebih kecil dari texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //

	glTexImage2D(GL_TEXTURE_2D, 0, 3, gambar2->sizeX, gambar2->sizeY, 0, GL_RGB,GL_UNSIGNED_BYTE, gambar2->data);


}

static void kibor(int key, int x, int y) {
	switch (key) {
	/*
	case GLUT_KEY_HOME: //up
		viewy++;
		break;
	case GLUT_KEY_END: //down
		viewy--;
		break;
	*/
	case GLUT_KEY_UP: //end
		viewx-=2;
		break;
	case GLUT_KEY_DOWN: //home
		viewx+=2;
		break;
	case GLUT_KEY_RIGHT:
		viewz-=2;
		break;
	case GLUT_KEY_LEFT:
		viewz+=2;
		break;

	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
    };
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	};
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {

		spin = spin - 2;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 2;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {
		viewz+=2;
	}
	if (key == 'e') {
		viewz-=2;
	}
	if (key == 's') {
		viewy-=2;
	}
	if (key == 'w') {
		viewy+=2;
	}
	if (key == 27) {
        exit(1);  // Escape key
	}

}

void reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Sample Terain");
	init();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(kibor);

	glutKeyboardFunc(keyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	glutMainLoop();
	return 0;
}
