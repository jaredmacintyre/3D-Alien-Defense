
/* Derived from scene.c in the The OpenGL Programming Guide */
/* Keyboard and mouse rotation taken from Swiftless Tutorials #23 Part 2 */
/* http://www.swiftless.com/tutorials/opengl/camera2.html */

/* Frames per second code taken from : */
/* http://www.lighthouse3d.com/opengl/glut/index.php?fps */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "graphics.h"

extern GLubyte  world[WORLDX][WORLDY][WORLDZ];

	/* mouse function called by GLUT when a button is pressed or released */
void mouse(int, int, int, int);

	/* initialize graphics library */
extern void graphicsInit(int *, char **);

	/* lighting control */
extern void setLightPosition(GLfloat, GLfloat, GLfloat);
extern GLfloat* getLightPosition();

	/* viewpoint control */
extern void setViewPosition(float, float, float);
extern void getViewPosition(float *, float *, float *);
extern void setOldViewPosition(float, float, float);
extern void getOldViewPosition(float *, float *, float *);
extern void setViewOrientation(float, float, float);
extern void getViewOrientation(float *, float *, float *);

	/* add cube to display list so it will be drawn */
extern void addDisplayList(int, int, int);

	/* mob controls */
extern void createMob(int, float, float, float, float);
extern void setMobPosition(int, float, float, float, float);
extern void hideMob(int);
extern void showMob(int);

	/* player controls */
extern void createPlayer(int, float, float, float, float);
extern void setPlayerPosition(int, float, float, float, float);
extern void hidePlayer(int);
extern void showPlayer(int);

	/* tube controls */
extern void createTube(int, float, float, float, float, float, float, int);
extern void getTubeStart(int, float *, float *, float *);
extern void getTubeEnd(int, float *, float *, float *);
extern void isTubeVisible(int, int *);
extern void hideTube(int);
extern void showTube(int);

	/* 2D drawing functions */
extern void  draw2Dline(int, int, int, int, int);
extern void  draw2Dbox(int, int, int, int);
extern void  draw2Dtriangle(int, int, int, int, int, int);
extern void  set2Dcolour(float []);

extern void isMoving(int *);
extern void getCurrentDirection(char *);


	/* flag which is set to 1 when flying behaviour is desired */
extern int flycontrol;
	/* flag used to indicate that the test world should be used */
extern int testWorld;
	/* flag to print out frames per second */
extern int fps;
	/* flag to indicate the space bar has been pressed */
extern int space;
	/* flag indicates the program is a client when set = 1 */
extern int netClient;
	/* flag indicates the program is a server when set = 1 */
extern int netServer; 
	/* size of the window in pixels */
extern int screenWidth, screenHeight;
	/* flag indicates if map is to be printed */
extern int displayMap;
	/* flag indicates use of a fixed viewpoint */
extern int fixedVP;

	/* frustum corner coordinates, used for visibility determination  */
extern float corners[4][3];

	/* determine which cubes are visible e.g. in view frustum */
extern void ExtractFrustum();
extern void tree(float, float, float, float, float, float, int);

	/* allows users to define colours */
extern int setUserColour(int, GLfloat, GLfloat, GLfloat, GLfloat, GLfloat,
    GLfloat, GLfloat, GLfloat);
void unsetUserColour(int);
extern void getUserColour(int, GLfloat *, GLfloat *, GLfloat *, GLfloat *,
    GLfloat *, GLfloat *, GLfloat *, GLfloat *); 

/********* end of extern variable declarations **************/

#define ACCEL_RATE 1.15 // acceleration rate
#define DECEL_RATE 0.9 // deceleration rate
#define VLIM 0.3 // max velocity limit
#define ALIEN_MAX 8 // number of aliens
#define HUMAN_MAX 8
#define BEAM_MAX 15
#define AVOID_MAX 12

struct timeval oldTime;
int updateTime = -1;

float oldMvt[3] = {0.0, 0.0, 0.0}; // old movement
int beamTick = 0; // beam tick
int tick = 0; // gravity tick
int first = 1;
float accel = 0.0;
int direction = ' ';
enum { INIT, SEARCH, MOVEDOWN, MOVEUP, SUCCESS, DEAD , END, AVOID, GROUND };

typedef struct Alien {
      int state;
      int previous, avoidCount;
      float x,y,z;
      float vecx, vecz;
      int target;
      float downFactor;
} Alien;

typedef struct Human {
      int x,y,z;
      int targeted;
      int dead;
      int fall;
} Human;

Alien alien[ALIEN_MAX];
Human human[HUMAN_MAX];

      /*** collisionResponse() ***/
      /* -performs collision detection and response */
      /*  sets new xyz  to position of the viewpoint after collision */
      /* -can also be used to implement gravity by updating y position of vp*/
      /* note that the world coordinates returned from getViewPosition()
      will be the negative value of the array indices */

void collisionResponse() {

	/* your code for collisions goes here */

      float newVP[3];
      getViewPosition(&newVP[0], &newVP[1], &newVP[2]);

      float oldVP[3];
      getOldViewPosition(&oldVP[0], &oldVP[1], &oldVP[2]);

      // float farVP[3];

      // Collision with world boundary
      if (newVP[0] < -99.9 || newVP[0] > 0.0) {
            setViewPosition(oldVP[0], newVP[1], newVP[2]);
      }

      if (newVP[1] < -50.0 || newVP[1] > 0.0) {
            setViewPosition(newVP[0], oldVP[1], newVP[2]);
      }

      if (newVP[2] < -99.9 || newVP[2] > 0.0) {
            setViewPosition(newVP[0], newVP[1], oldVP[2]);
      }

      // add padding to new viewposition
      float pad = 0.15;
      float padVP[3];
      for (int i=0; i<3; i++) {
            if (newVP[i] - oldVP[i] < 0.0) padVP[i] = newVP[i] - pad;
            else padVP[i] = newVP[i] + pad;
      }

      // Collision with item in world[][][]
      if (world[abs((int)padVP[0])][abs((int)padVP[1])][abs((int)padVP[2])] > 0) {
            // printf("Collision\n");
            setViewPosition(oldVP[0], oldVP[1], oldVP[2]);
      }
}


	/******* draw2D() *******/
	/* draws 2D shapes on screen */
	/* use the following functions: 			*/
	/*	draw2Dline(int, int, int, int, int);		*/
	/*	draw2Dbox(int, int, int, int);			*/
	/*	draw2Dtriangle(int, int, int, int, int, int);	*/
	/*	set2Dcolour(float []); 				*/
	/* colour must be set before other functions are called	*/
void draw2D() {

   if (testWorld) {
		/* draw some sample 2d shapes */
      if (displayMap == 1) {
         GLfloat green[] = {0.0, 0.5, 0.0, 0.5};
         set2Dcolour(green);
         draw2Dline(0, 0, 500, 500, 15);
         draw2Dtriangle(0, 0, 200, 200, 0, 200);

         GLfloat black[] = {0.0, 0.0, 0.0, 0.5};
         set2Dcolour(black);
         draw2Dbox(500, 380, 524, 388);
      }
   } else {
	/* your code goes here */
      GLfloat pink[] = {1.0, 0.3, 0.5, 1.5};
      GLfloat black[] = {0.0, 0.0, 0.0, 0.7};
      GLfloat green[] = {0.0, 0.5, 0.0, 1.5};
      GLfloat red[] = {1.0, 0.0, 0.0, 1.5};
      GLfloat blue[] = {0.0, 0.0, 1.0, 1.5};
      GLfloat yellow[] = {1.0, 0.4, 0.0, 1.5};
      GLfloat forest[] = {0.0, 0.25, 0.0, 1.5};

      if (displayMap == 0) return;

      if (displayMap == 1) {
            // Small Map
            float scale = screenHeight*screenWidth*0.00025;
            float x1 = screenWidth-scale;
            float y1 = screenHeight-0.02*scale;
            float x2 = screenWidth-0.02*scale;
            float y2 = screenHeight-scale;
            float mWidth = x2-x1;
            float mHeight = y1-y2;
            int humanCount = 0;

            // printf("outline: (%f,%f,%f,%f)\n",x1,y1,x2,y2);
            // printf("map: (%f,%f)\n",mWidth,mHeight);
            
            // player icon
            float oldVP[3]; // old viewposition
            getOldViewPosition(&oldVP[0], &oldVP[1], &oldVP[2]);

            float px = (( (float) WORLDX - fabs(oldVP[0]) ) / (float) WORLDX) * mWidth;
            // float pz = (( (float) WORLDZ - fabs(oldVP[2]) ) / (float) WORLDZ) * mHeight;
            // float px = (( fabs(oldVP[0]) ) / (float) WORLDX) * mWidth;
            float pz = (( fabs(oldVP[2]) ) / (float) WORLDZ) * mHeight;
            float player[2] = { x1+px , y2+pz };
            float ax, ay, bx, by, cx, cy;
            cx = player[0];
            cy = player[1] + 0.03 * scale;
            bx = player[0] - 0.03 * scale;
            by = player[1] - 0.03 * scale;
            ax = player[0] + 0.03 * scale;
            ay = player[1] - 0.03 * scale;
            // printf("triangle\na:[%f][%f]\nb: [%f][%f]\nc: [%f][%f]\n", ax, ay, bx, by, cx, cy);
            set2Dcolour(red);
            // draw2Dbox(x1+px+0.04*scale, y2+pz-0.04*scale, x1+px-0.04*scale, y2+pz+0.04*scale);
            draw2Dtriangle(ax, ay, bx, by, cx, cy);

            // beam (ray)
            for (int i=1; i<=8; i++) {
                  int visible;
                  isTubeVisible(i, &visible);
                  if (visible == 1) {
                        // get tube coordinates
                        float bx, by, bz, ex, ey, ez;
                        getTubeStart(i, &bx, &by, &bz);
                        getTubeEnd(i, &ex, &ey, &ez);
                        // scale to map
                        float mstartx = (((float) WORLDX - bx) / (float) WORLDX) * mWidth;
                        float mstarty = (bz / (float) WORLDZ) * mHeight;
                        float mendx = (((float) WORLDX - ex)  / (float) WORLDX) * mWidth;
                        float mendy = (ez / (float) WORLDZ) * mHeight;
                        // printf("beam\nstart:[%f][%f]\nend:[%f][%f]\n", x1+mstartx, y2+mstarty, x1+mendx, y2+mendy);
                        // draw
                        set2Dcolour(blue);
                        draw2Dline(x1+mstartx, y2+mstarty, x1+mendx, y2+mendy, (int)(scale*0.02));
                  }
            }

            // alien icons
            for(int x=0; x<WORLDX; x++) {
                  for(int y=0; y<WORLDY; y++) {
                        for(int z=0; z<WORLDZ; z++) {
                              if (world[x][y][z] == 12) {
                                    // Human
                                    float px = ((float) (WORLDX-x) / (float) WORLDX) * mWidth;
                                    // float pz = ((float) (WORLDZ-z) / (float) WORLDZ) * mHeight;
                                    // float px = ((float) (x) / (float) WORLDX) * mWidth;
                                    float pz = ((float) (z) / (float) WORLDZ) * mHeight;
                                    // printf("pxpz: (%f,%f)\n",px,pz);
                                    set2Dcolour(yellow);
                                    draw2Dbox(x1+px+0.01*scale, y2+pz-0.01*scale, x1+px-0.01*scale, y2+pz+0.01*scale);
                                    set2Dcolour(forest);
                                    draw2Dbox(x1+px+0.025*scale, y2+pz-0.025*scale, x1+px-0.025*scale, y2+pz+0.025*scale);
                                    // printf("hooman: (%f,%f)\n", x1+px, y2+pz-2);
                                    humanCount++;
                                    break;
                              }
                        }
                  }
            }
            
            // human icons
            for(int x=0; x<WORLDX; x++) {
                  for(int y=0; y<WORLDY; y++) {
                        for(int z=0; z<WORLDZ; z++) {
                              if (world[x][y][z] == 1) {
                                    // Human
                                    float px = ((float) (WORLDX-x) / (float) WORLDX) * mWidth;
                                    // float pz = ((float) (WORLDZ-z) / (float) WORLDZ) * mHeight;
                                    // float px = ((float) (x) / (float) WORLDX) * mWidth;
                                    float pz = ((float) (z) / (float) WORLDZ) * mHeight;
                                    // printf("pxpz: (%f,%f)\n",px,pz);
                                    set2Dcolour(green);
                                    draw2Dbox(x1+px+0.01*scale, y2+pz-0.01*scale, x1+px-0.01*scale, y2+pz+0.01*scale);
                                    // printf("hooman: (%f,%f)\n", x1+px, y2+pz-2);
                                    humanCount++;
                                    break;
                              }
                        }
                  }
            }
            // background
            set2Dcolour(black);
            draw2Dbox(x1, y1, x2, y2);
      }
      if (displayMap == 2) {
            // Large Map
            float scale = screenHeight*screenWidth*0.0002;
            float x1 = screenWidth/2 - scale*1.3;
            float y1 = screenHeight/2 + scale*1.3;
            float x2 = screenWidth/2 + scale*1.3;
            float y2 = screenHeight/2 - scale*1.3;
            float mWidth = x2-x1;
            float mHeight = y1-y2;
            int humanCount = 0;

            // printf("outline: (%f,%f,%f,%f)\n",x1,y1,x2,y2);
            // printf("map: (%f,%f)\n",mWidth,mHeight);
            
            // player icon
            float oldVP[3]; // old viewposition
            getOldViewPosition(&oldVP[0], &oldVP[1], &oldVP[2]);

            float px = (( (float) WORLDX - fabs(oldVP[0]) ) / (float) WORLDX) * mWidth;
            // float pz = (( (float) WORLDZ - fabs(oldVP[2]) ) / (float) WORLDZ) * mHeight;
            // float px = (( fabs(oldVP[0]) ) / (float) WORLDX) * mWidth;
            float pz = (( fabs(oldVP[2]) ) / (float) WORLDZ) * mHeight;
            float player[2] = { x1+px , y2+pz };
            float ax, ay, bx, by, cx, cy;
            cx = player[0];
            cy = player[1] + 0.06 * scale;
            bx = player[0] - 0.06 * scale;
            by = player[1] - 0.06 * scale;
            ax = player[0] + 0.06 * scale;
            ay = player[1] - 0.06 * scale;
            // printf("scale %f", scale);
            // printf("triangle\na:[%f][%f]\nb: [%f][%f]\nc: [%f][%f]\n", ax, ay, bx, by, cx, cy);
            set2Dcolour(red);
            // draw2Dbox(x1+px+0.04*scale, y2+pz-0.04*scale, x1+px-0.04*scale, y2+pz+0.04*scale);
            draw2Dtriangle(ax, ay, bx, by, cx, cy);

            // beam (ray)
            for (int i=1; i<=8; i++) {
                  int visible;
                  isTubeVisible(i, &visible);
                  if (visible == 1) {
                        // get tube coordinates
                        float bx, by, bz, ex, ey, ez;
                        getTubeStart(i, &bx, &by, &bz);
                        getTubeEnd(i, &ex, &ey, &ez);
                        // scale to map
                        float mstartx = (((float) WORLDX - bx) / (float) WORLDX) * mWidth;
                        float mstarty = (bz / (float) WORLDZ) * mHeight;
                        float mendx = (((float) WORLDX - ex)  / (float) WORLDX) * mWidth;
                        float mendy = (ez / (float) WORLDZ) * mHeight;
                        // draw
                        set2Dcolour(blue);
                        draw2Dline(x1+mstartx, y2+mstarty, x1+mendx, y2+mendy, (int)(scale*0.04));
                  }
            }

            // alien icons
            for(int x=0; x<WORLDX; x++) {
                  for(int y=0; y<WORLDY; y++) {
                        for(int z=0; z<WORLDZ; z++) {
                              if (world[x][y][z] == 12) {
                                    // Human
                                    float px = ((float) (WORLDX-x) / (float) WORLDX) * mWidth;
                                    // float pz = ((float) (WORLDZ-z) / (float) WORLDZ) * mHeight;
                                    // float px = ((float) (x) / (float) WORLDX) * mWidth;
                                    float pz = ((float) (z) / (float) WORLDZ) * mHeight;
                                    // printf("pxpz: (%f,%f)\n",px,pz);
                                    set2Dcolour(yellow);
                                    draw2Dbox(x1+px+0.03*scale, y2+pz-0.03*scale, x1+px-0.03*scale, y2+pz+0.03*scale);
                                    set2Dcolour(forest);
                                    draw2Dbox(x1+px+0.075*scale, y2+pz-0.075*scale, x1+px-0.075*scale, y2+pz+0.075*scale);
                                    // printf("hooman: (%f,%f)\n", x1+px, y2+pz-2);
                                    humanCount++;
                                    break;
                              }
                        }
                  }
            }

            // human icons
            for(int x=0; x<WORLDX; x++) {
                  for(int y=0; y<WORLDY; y++) {
                        for(int z=0; z<WORLDZ; z++) {
                              if (world[x][y][z] == 1) {
                                    // Human
                                    float px = ((float) (WORLDX-x) / (float) WORLDX) * mWidth;
                                    // float pz = ((float) (WORLDZ-z) / (float) WORLDZ) * mHeight;
                                    // float px = ((float) (x) / (float) WORLDX) * mWidth;
                                    float pz = ((float) (z) / (float) WORLDZ) * mHeight;
                                    // printf("pxpz: (%f,%f)\n",px,pz);
                                    set2Dcolour(green);
                                    draw2Dbox(x1+px+0.03*scale, y2+pz-0.03*scale, x1+px-0.03*scale, y2+pz+0.03*scale);
                                    // printf("hooman: (%f,%f)\n", x1+px, y2+pz-2);
                                    humanCount++;
                                    break;
                              }
                        }
                  }
            }

            // background
            set2Dcolour(black);
            draw2Dbox(x1, y1, x2, y2);
      }
   }
}

/* returns 1 if ViewPositions are equivalent */
int compareVP (float vp1[3], float vp2[3])  {
      for (int i=0; i<3; i++) {
            if (vp1[i] != vp2[i]) return 0;
      }
      return 1;
}

void limit (float *x, float *y, float *z) {
      printf("origi : [%.4f][%.4f][%.4f]\n", *x, *y, *z);
      float magnitude = sqrtf( powf(*x,2) + powf(*y,2) + powf(*z,2) );
      if (magnitude < VLIM) return;
      if (*x == 0.0) {
            if (*y == 0.0) {
                  if (*z > 0.0) *z = VLIM;
                  else *z = -VLIM ;
            }
            else {
                  float zrate = *z / fabs(*y);
                  if (*y > 0.0) *y = VLIM / sqrtf( 1 + powf(*z / *y,2) );
                  else *y = -VLIM / sqrtf( 1 + powf(*z / *y,2) );
                  *z = *y * zrate;
            }
      }
      else {
            float yrate = *y / fabs(*x);
            float zrate = *z / fabs(*x);
            if (*x > 0.0) *x = VLIM / sqrtf( 1 + powf(*y / *x,2) + powf(*z / *x,2) );
            else *x = -VLIM / sqrtf( 1 + powf(*y / *x,2) + powf(*z / *x,2) );
            *y = *x * yrate;
            *z = *x * zrate;
      }
      printf("limit : [%.4f][%.4f][%.4f]\n", *x, *y, *z);
}

void speedlimit (float *x, float *y, float *z) {
      if (*x > VLIM) *x = VLIM;
      if (*x < -VLIM) *x = -VLIM;
      if (*y > VLIM) *y = VLIM;
      if (*y < -VLIM) *y = -VLIM;
      if (*z > VLIM) *z = VLIM;
      if (*z < -VLIM) *z = -VLIM;
}

void clearAlien(int x, int y, int z) {
      // top
      world[x][y+1][z] = 0;
      // middle
      // world[x][y][z] = 1;
      world[x][y][z+1] = 0;
      world[x][y][z-1] = 0;
      world[x+1][y][z] = 0;
      world[x-1][y][z] = 0;

      world[x+1][y][z-1] = 0;
      world[x+1][y][z+1] = 0;
      world[x-1][y][z-1] = 0;
      world[x-1][y][z+1] = 0;
      // bottom
      world[x+1][y-1][z-1] = 0;
      world[x+1][y-1][z+1] = 0;
      world[x-1][y-1][z-1] = 0;
      world[x-1][y-1][z+1] = 0;
}

void drawAlien(int x, int y, int z) {
      // top
      world[x][y+1][z] = 12;
      // middle
      // world[x][y][z] = 1;
      world[x][y][z+1] = 11;
      world[x][y][z-1] = 11;
      world[x+1][y][z] = 11;
      world[x-1][y][z] = 11;

      world[x+1][y][z-1] = 11;
      world[x+1][y][z+1] = 11;
      world[x-1][y][z-1] = 11;
      world[x-1][y][z+1] = 11;
      // bottom
      world[x+1][y-1][z-1] = 11;
      world[x+1][y-1][z+1] = 11;
      world[x-1][y-1][z-1] = 11;
      world[x-1][y-1][z+1] = 11;
}

void alienCollision(int a) {
      for (int i=-3; i<=3; i++) {
            for (int j=-3; j<=3; j++) {
                  for (int k=-3; k<=3; k++) {
                        for (int l=0; l<ALIEN_MAX; l++) {
                              // if (i == 0 && j == 0 && ) continue;
                              if (l == a) continue;
                              if ((int) alien[a].x + i == (int) alien[l].x && (int) alien[a].y + j == (int) alien[l].y && (int) alien[a].z + k == (int) alien[l].z) {
                                    if (alien[a].state != AVOID) alien[a].previous = alien[a].state;
                                    alien[a].state = AVOID;
                                    alien[a].avoidCount = AVOID_MAX;
                                    return;
                              }
                        }
                  }
            }
      }
      return;
}

void groundCollision(int a) {
      for (int i=-3; i<=3; i++) {
            for (int j=-3; j<=3; j++) {
                  for (int k=-3; k<=3; k++) {
                        if (world[(int) alien[a].x + i][(int) alien[a].y + j][(int) alien[a].z + k] == 9) {
                              alien[a].state = GROUND;
                              alien[a].avoidCount = AVOID_MAX;
                              return;
                        }
                  }
            }
      }
      return;
}

/* Alien State Machine */
void alienControl () {
      // alien collision check
      for (int i=0; i<ALIEN_MAX; i++) {
            if (alien[i].state == SEARCH || alien[i].state == MOVEDOWN || alien[i].state == MOVEUP)
                  if (alien[i].avoidCount == 0)  
                        alienCollision(i);
      }
      // ground collision check
      // state machine
      for (int i=0; i<ALIEN_MAX; i++) {
            switch (alien[i].state) {
                  case INIT: {
                        // generate init position
                        alien[i].x = (float) (rand() % 80 + 10); // pseudo-random x coordinate
                        alien[i].y = 40.0;
                        alien[i].z = (float) (rand() % 80 + 10); // pseudo-random z coordinate
                        drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        // initialize movement velocity
                        alien[i].vecx = ((float) rand() / (float) (RAND_MAX)) * 0.15 + 0.05;
                        alien[i].vecz = ((float) rand() / (float) (RAND_MAX)) * 0.15 + 0.05;
                        // init target
                        alien[i].target = -1;
                        // start searching
                        alien[i].state = SEARCH;
                        // avoid animation counter
                        alien[i].avoidCount = 0;
                        break;
                  }
                  case SEARCH: {
                        // switch direction at edge of world
                        if (alien[i].x + alien[i].vecx > 97.0 || alien[i].x + alien[i].vecx < 2.0) alien[i].vecx = -(alien[i].vecx);
                        if (alien[i].z + alien[i].vecz > 97.0 || alien[i].z + alien[i].vecz < 2.0) alien[i].vecz = -(alien[i].vecz);
                        // move alien
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        if (alien[i].y < 40.0) {
                              if (40.0 - alien[i].y > 0.2) alien[i].y += 0.15;
                              else alien[i].y = 40.0;
                        }
                        else if (alien[i].y > 40.0) {
                              if (alien[i].y - 40.0 > 0.2) alien[i].y -= 0.15;
                              else alien[i].y = 40.0;
                        }
                        else {
                              alien[i].x  += alien[i].vecx;
                              alien[i].z  += alien[i].vecz;
                        }
                        drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        // look for available humans
                        for (int j=0; j<8; j++) {
                              for (int k=0; k<8-i; k++) {
                                    for (int y=5; y<15; y++) {
                                          if (world[(int) alien[i].x + j][y][(int) alien[i].z + k] == 1) {
                                                // check if targeted
                                                int newTarget[3] = { (int) alien[i].x + j, y, (int) alien[i].z + k };
                                                for (int n=0; n<HUMAN_MAX; n++) {
                                                      if (newTarget[0] == human[n].x && newTarget[2] == human[n].z) {
                                                            if (human[n].targeted < 0){
                                                                  alien[i].target = n;
                                                                  alien[i].state = MOVEDOWN;
                                                                  human[n].targeted = i;
                                                            }
                                                            break;
                                                      }
                                                }
                                                break;
                                          }
                                          if (world[(int) alien[i].x + j][y][(int) alien[i].z - k] == 1) {
                                                int newTarget[3] = { (int) alien[i].x + j, y, (int) alien[i].z - k };
                                                for (int n=0; n<HUMAN_MAX; n++) {
                                                      if (newTarget[0] == human[n].x && newTarget[2] == human[n].z) {
                                                            if (human[n].targeted < 0){
                                                                  alien[i].target = n;
                                                                  alien[i].state = MOVEDOWN;
                                                                  human[n].targeted = i;
                                                            }
                                                            break;
                                                      }
                                                }
                                                break;
                                          }
                                          if (world[(int) alien[i].x - j][y][(int) alien[i].z + k] == 1) {
                                                int newTarget[3] = { (int) alien[i].x - j, y, (int) alien[i].z + k };
                                                for (int n=0; n<HUMAN_MAX; n++) {
                                                      if (newTarget[0] == human[n].x && newTarget[2] == human[n].z) {
                                                            if (human[n].targeted < 0){
                                                                  alien[i].target = n;
                                                                  alien[i].state = MOVEDOWN;
                                                                  human[n].targeted = i;
                                                            }
                                                            break;
                                                      }
                                                }
                                                break;
                                          }
                                          if (world[(int) alien[i].x - j][y][(int) alien[i].z - k] == 1){
                                                int newTarget[3] = { (int) alien[i].x - j, y, (int) alien[i].z - k };
                                                for (int n=0; n<HUMAN_MAX; n++) {
                                                      if (newTarget[0] == human[n].x && newTarget[2] == human[n].z) {
                                                            if (human[n].targeted < 0){
                                                                  alien[i].target = n;
                                                                  alien[i].state = MOVEDOWN;
                                                                  human[n].targeted = i;
                                                            }
                                                            break;
                                                      }
                                                }
                                                break;
                                          }
                                    }
                              }
                        }
                        break;
                  }
                  case MOVEDOWN: {
                        // check if target moved
                        // if (world[human[alien[i].target].x][human[alien[i].target].y+2][human[alien[i].target].z] == 0) {
                        //       alien[i].target[1] -= 1;
                        // }
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        alien[i].downFactor = 0.1 / sqrtf(
                              powf(human[alien[i].target].x - alien[i].x, 2) + 
                              powf(human[alien[i].target].y+4 - alien[i].y, 2) + 
                              powf(human[alien[i].target].z - alien[i].z, 2)
                        );
                        alien[i].x  += (human[alien[i].target].x - alien[i].x) * alien[i].downFactor;
                        alien[i].y  += (human[alien[i].target].y+4 - alien[i].y) * alien[i].downFactor;
                        alien[i].z  += (human[alien[i].target].z - alien[i].z) * alien[i].downFactor;
                        drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);

                        if (sqrtf(powf(human[alien[i].target].x - alien[i].x, 2) + powf(human[alien[i].target].y+4 - alien[i].y, 2) + powf(human[alien[i].target].z - alien[i].z, 2)) < 0.1) {
                              clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                              alien[i].x  = human[alien[i].target].x;
                              alien[i].y  = human[alien[i].target].y+4;
                              alien[i].z  = human[alien[i].target].z;
                              drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        }

                        if ((int) alien[i].x == human[alien[i].target].x && (int) alien[i].y == human[alien[i].target].y+4 && (int) alien[i].z == human[alien[i].target].z)
                              alien[i].state = MOVEUP;
                        break;
                  }
                  case MOVEUP: {
                        // move alien
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        alien[i].y  += 0.04;
                        drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        // move human
                        world[(int) alien[i].x][(int) alien[i].y-2][(int) alien[i].z] = 1;
                        world[(int) alien[i].x][(int) alien[i].y-3][(int) alien[i].z] = 3;
                        world[(int) alien[i].x][(int) alien[i].y-4][(int) alien[i].z] = 7;
                        human[alien[i].target].y = (int) alien[i].y-4;
                        if (world[(int) alien[i].x][(int) alien[i].y-5][(int) alien[i].z] == 7) world[(int) alien[i].x][(int) alien[i].y-5][(int) alien[i].z] = 0;
                        // top of world is reached
                        if (alien[i].y > 48)
                              alien[i].state = SUCCESS;
                        break;
                  }
                  case SUCCESS: {
                        // remove alien and human
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        world[(int) alien[i].x][(int) alien[i].y-2][(int) alien[i].z] = 0;
                        world[(int) alien[i].x][(int) alien[i].y-3][(int) alien[i].z] = 0;
                        world[(int) alien[i].x][(int) alien[i].y-4][(int) alien[i].z] = 0;
                        // printf("Alien escaped\n");
                        printf("Human was lost\n");
                        human[alien[i].target].targeted = -1;
                        human[alien[i].target].dead = 1;
                        alien[i].state = END;
                        break;
                  }
                  case DEAD: {
                        // remove alien
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        if (alien[i].target > -1) 
                              human[alien[i].target].targeted = -1;
                        alien[i].target = -1;
                        alien[i].state = END;
                        break;
                  }
                  case END: {
                        break;
                  }
                  case AVOID: {
                        switch (alien[i].previous) {
                              case SEARCH: {
                                    if (alien[i].avoidCount == AVOID_MAX) {
                                          alien[i].vecx = -(alien[i].vecx);
                                          alien[i].vecz = -(alien[i].vecz);
                                    }
                                    // switch direction at edge of world
                                    if (alien[i].x + alien[i].vecx > 97.0 || alien[i].x + alien[i].vecx < 2.0) alien[i].vecx = -(alien[i].vecx);
                                    if (alien[i].z + alien[i].vecz > 97.0 || alien[i].z + alien[i].vecz < 2.0) alien[i].vecz = -(alien[i].vecz);
                                    // move alien
                                    clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    alien[i].x += alien[i].vecx;
                                    alien[i].z += alien[i].vecz;
                                    drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    break;
                              }
                              case MOVEDOWN: {
                                    clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    alien[i].y += 0.15;
                                    drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    break;
                              }
                              case MOVEUP: {
                                    clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    alien[i].y -= 0.15;
                                    drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                                    break;
                              }
                              default: {
                                    printf("what have i done\n");
                                    printf("%d\n", alien[i].previous);
                                    break;
                              }
                        }

                        if (alien[i].avoidCount != 0) alien[i].avoidCount--;
                        else alien[i].state = alien[i].previous;
                        break;
                  }
                  case GROUND: {
                        clearAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);
                        alien[i].y += 0.15;
                        drawAlien((int) alien[i].x, (int) alien[i].y, (int) alien[i].z);

                        if (alien[i].avoidCount != 0) alien[i].avoidCount--;
                        else alien[i].state = alien[i].previous;
                        break;
                  }
            }
      }
}

	/*** update() ***/
	/* background process, it is called when there are no other events */
	/* -used to control animations and perform calculations while the  */
	/*  system is running */
	/* -gravity must also implemented here, duplicate collisionResponse */
void update() {
int i, j, k;
float *la;
float x, y, z;

	/* sample animation for the test world, don't remove this code */
	/* demo of animating mobs */
   if (testWorld) {


	/* update old position so it contains the correct value */
	/* -otherwise view position is only correct after a key is */
	/*  pressed and keyboard() executes. */
      getViewPosition(&x, &y, &z);
      setOldViewPosition(x,y,z);

	/* sample of rotation and positioning of mob */
	/* coordinates for mob 0 */
      static float mob0x = 50.0, mob0y = 25.0, mob0z = 52.0;
      static float mob0ry = 0.0;
      static int increasingmob0 = 1;
	/* coordinates for mob 1 */
      static float mob1x = 50.0, mob1y = 25.0, mob1z = 52.0;
      static float mob1ry = 0.0;
      static int increasingmob1 = 1;
	/* counter for user defined colour changes */
      static int colourCount = 0;
      static GLfloat offset = 0.0;

	/* move mob 0 and rotate */
	/* set mob 0 position */
      setMobPosition(0, mob0x, mob0y, mob0z, mob0ry);

	/* move mob 0 in the x axis */
      if (increasingmob0 == 1)
         mob0x += 0.2;
      else 
         mob0x -= 0.2;
      if (mob0x > 50) increasingmob0 = 0;
      if (mob0x < 30) increasingmob0 = 1;

	/* rotate mob 0 around the y axis */
      mob0ry += 1.0;
      if (mob0ry > 360.0) mob0ry -= 360.0;

	/* move mob 1 and rotate */
      setMobPosition(1, mob1x, mob1y, mob1z, mob1ry);

	/* move mob 1 in the z axis */
	/* when mob is moving away it is visible, when moving back it */
	/* is hidden */
      if (increasingmob1 == 1) {
         mob1z += 0.2;
         showMob(1);
      } else {
         mob1z -= 0.2;
         hideMob(1);
      }
      if (mob1z > 72) increasingmob1 = 0;
      if (mob1z < 52) increasingmob1 = 1;

	/* rotate mob 1 around the y axis */
      mob1ry += 1.0;
      if (mob1ry > 360.0) mob1ry -= 360.0;

	/* change user defined colour over time */
      if (colourCount == 1) offset += 0.05;
      else offset -= 0.01;
      if (offset >= 0.5) colourCount = 0;
      if (offset <= 0.0) colourCount = 1;
      setUserColour(9, 0.7, 0.3 + offset, 0.7, 1.0, 0.3, 0.15 + offset, 0.3, 1.0);

	/* sample tube creation  */
	/* draws a purple tube above the other sample objects */
       createTube(1, 45.0, 30.0, 45.0, 50.0, 30.0, 50.0, 6);

    /* end testworld animation */

   } else {
	/* your code goes here */

      //////////////// TIMING ////////////////
      struct timeval newTime;
      gettimeofday(&newTime, NULL);

      if (updateTime != -1) {
            unsigned long diff = (newTime.tv_sec - oldTime.tv_sec) * 1000000 + (newTime.tv_usec - oldTime.tv_usec);
            // printf("%ld microseconds\n", diff);

            // Only run if last update over 16000 milliseconds
            if (diff > 16000) {
                  // printf("update()\n");
                  oldTime = newTime;
            }
            else return;
      }
      else oldTime = newTime;

      updateTime = 0;

      

      float newMvt[3]; // new movement

      float newVP[3]; // new viewposition
      getViewPosition(&newVP[0], &newVP[1], &newVP[2]);

      float oldVP[3]; // old viewposition
      getOldViewPosition(&oldVP[0], &oldVP[1], &oldVP[2]);
      // resolve init oldVP case
      for (int i=0; i<3; i++) {
            if (oldVP[i] == 0.0000000) {
                  oldVP[i] = -50.0;
                  setOldViewPosition(newVP[0], newVP[1], newVP[2]);
            }
      }

      // printf("DEBUG\n");
      // printf("oldVP1[%f][%f][%f]\n", oldVP[0], oldVP[1], oldVP[2]); // debug
      // printf("newVP1[%f][%f][%f]\n", newVP[0], newVP[1], newVP[2]); // debug

      // check if moving
      int moving;
      isMoving(&moving);
     
      if (moving == 0) {
            // NOT MOVING
            // printf("Not moving\n");
            // calculate deceleration amount
            for (int i=0; i<3; i++) {
                  newMvt[i] = oldMvt[i] * DECEL_RATE; // deceleration factor
            }
            // calculate 
            accel = 0.1;
      }
      else {
            // MOVING
            // printf("Moving\n");
            char newD;
            getCurrentDirection(&newD);
      
            // calculate acceleration amount
            for (int i=0; i<3; i++) {
                  if (compareVP(newVP, oldVP) == 1) {
                        newMvt[i] = oldMvt[i];
                  }
                  else {
                        newMvt[i] = (newVP[i] - oldVP[i]) * 0.8;                            
                  }
                  // newMvt[i] = (newVP[i] - oldVP[i]) * 0.05 + (oldMvt[i] * ACCEL_RATE);
            }

            if (newD != direction) {
                  direction = newD;
            }
      }

      // limit(&newMvt[0], &newMvt[1], &newMvt[2]);

      // printf("oldMvt[%f][%f][%f]\n", oldMvt[0], oldMvt[1], oldMvt[2]); // debug
      // printf("newMvt[%f][%f][%f]\n", newMvt[0], newMvt[1], newMvt[2]); // debug

      // store movement size
      for (int i=0; i<3; i++) {
            oldMvt[i] = newMvt[i];
            if (fabs(oldMvt[i]) < 0.0000001) oldMvt[i] = 0.0;
            
      }

      // update oldviewposition
      getViewPosition(&newVP[0], &newVP[1], &newVP[2]);
      setOldViewPosition(newVP[0], newVP[1], newVP[2]);

      // update viewposition
      setViewPosition(oldVP[0]+newMvt[0], oldVP[1]+newMvt[1], oldVP[2]+newMvt[2]);

      collisionResponse();

      // update oldviewposition
      getViewPosition(&newVP[0], &newVP[1], &newVP[2]);
      setOldViewPosition(newVP[0], newVP[1], newVP[2]);

      getViewPosition(&newVP[0], &newVP[1], &newVP[2]);
      getOldViewPosition(&newVP[0], &newVP[1], &newVP[2]);
      // printf("oldVP2[%f][%f][%f]\n", oldVP[0], oldVP[1], oldVP[2]); // debug
      // printf("newVP2[%f][%f][%f]\n", newVP[0], newVP[1], newVP[2]); // debug
      // printf("---------------------------------------------\n");    // debug

      // alien
      alienControl();

      // gravity
      if (tick < 15) tick++;
      else tick = 0;
      if (tick == 0) {
            for (int i=0; i<HUMAN_MAX; i++) {
                  // apply gravity
                  if (human[i].targeted < 0 && human[i].dead != 1 && world[human[i].x][human[i].y-1][human[i].z] != 9) {
                        world[human[i].x][human[i].y-1][human[i].z] = 7;
                        world[human[i].x][human[i].y][human[i].z] = 3;
                        world[human[i].x][human[i].y+1][human[i].z] = 1;
                        world[human[i].x][human[i].y+2][human[i].z] = 0;
                        human[i].y -= 1;
                        human[i].fall += 1;
                  }
                  // calculate fall damage
                  else if (human[i].dead != 1 && world[human[i].x][human[i].y-1][human[i].z] == 9) {
                        if (human[i].fall > 20) {
                              // kill human
                              printf("Human was lost\n");
                              // set alien to SEARCH
                              if (human[i].targeted > -1) {
                                    alien[human[i].targeted].target = -1;
                                    alien[human[i].targeted].state = SEARCH;
                              }
                              // clear human
                              for (int y=0; y<50; y++) {
                                    if (world[human[i].x][y][human[i].z] == 1 || world[human[i].x][y][human[i].z] == 3 || world[human[i].x][y][human[i].z] == 7)
                                          world[human[i].x][y][human[i].z] = 0;
                              }
                              human[i].targeted = -1;
                              human[i].dead = 1;
                        }
                        human[i].fall = 0;
                  }
            }
      }

      // beam
      if (beamTick > 0) {
            beamTick--;
            if (beamTick == 0) {
                  for (int i=1; i<=BEAM_MAX; i++)
                        hideTube(i);
            }
      }
   }
}


	/* called by GLUT when a mouse button is pressed or released */
	/* -button indicates which button was pressed or released */
	/* -state indicates a button down or button up event */
	/* -x,y are the screen coordinates when the mouse is pressed or */
	/*  released */ 
void mouse(int button, int state, int x, int y) {

   if (button == GLUT_LEFT_BUTTON) {
      // printf("left button - ");
      float VO[3];
      getViewOrientation(&VO[0], &VO[1], &VO[2]);  
      for (int i=0; i<3; i++) {
            VO[i] = fmod(VO[i], 360);
      }
      // printf("ViewOrientation (%f, %f, %f)\n", VO[0], VO[1], VO[2]);
      float dir[3];
      float val = M_PI / 180.0;
      dir[0] = sin(VO[1] * val);
      dir[1] = -sin(VO[0] * val);
      dir[2] = -cos(VO[1] * val);
      // printf("ViewRadians (%f, %f)\n", VO[0] * val, VO[1] * val);
      // printf("ViewDirection (%f, %f, %f)\n", dir[0], dir[1], dir[2]);
      float start[3]; // start of beam
      getViewPosition(&start[0], &start[1], &start[2]);
      for (int i=0; i<3; i++) start[i] = fabs(start[i]);

      // float end[3]; // end of beam
      // for (int i=0; i<3; i++) end[i] = start[i] + 4*dir[i];
      // printf("------------\n");
      // printf("start (%f, %f, %f)\n", start[0], start[1], start[2]);
      // printf("dir (%f, %f, %f)\n", dir[0], dir[1], dir[2]);
      // printf("end (%f, %f, %f)\n", end[0], end[1], end[2]);
      // printf("------------\n");

      /* creates blue beam */
      for (int i=0; i<BEAM_MAX; i++) {
            createTube(i+1, 
            start[0] + i*dir[0], 
            start[1] + i*dir[1], 
            start[2] + i*dir[2], 
            start[0] + (i+1)*dir[0], 
            start[1] + (i+1)*dir[1], 
            start[2] + (i+1)*dir[2], 
            2);
      }
      // beam collision
      for (int i=1; i<=BEAM_MAX; i++) {
            float end[3];
            getTubeEnd(i, &end[0], &end[1], &end[2]);
            // beam collision with ALIEN
            if (world[(int)end[0]][(int)end[1]][(int)end[2]] == 11 || world[(int)end[0]][(int)end[1]][(int)end[2]] == 12) {
                  if (state == GLUT_UP) {
                        printf("Alien killed\n");
                        // set alien to DEAD
                        for (int j=0; j<ALIEN_MAX; j++) {
                              if (fabs(end[0] - alien[j].x) < 2 && fabs(end[1] - alien[j].y) < 2 && fabs(end[2] - alien[j].z) < 2) 
                                    alien[j].state = DEAD;
                        }
                        break;
                  }
            }
            // beam collision with HUMAN
            if (world[(int)end[0]][(int)end[1]][(int)end[2]] == 1 || world[(int)end[0]][(int)end[1]][(int)end[2]] == 3 || world[(int)end[0]][(int)end[1]][(int)end[2]] == 7) {
                  if (state == GLUT_UP) {
                        printf("Human was lost\n");
                        for (int j=0; j<HUMAN_MAX; j++) {
                              if ((int)end[0] == human[j].x && (int)end[2] == human[j].z) {
                                    // set alien to SEARCH
                                    if (human[j].targeted > -1) {
                                          alien[human[j].targeted].target = -1;
                                          alien[human[j].targeted].state = SEARCH;
                                    }
                                    // clear human
                                    for (int y=0; y<50; y++) {
                                          if (world[human[j].x][y][human[j].z] == 1 || world[human[j].x][y][human[j].z] == 3 || world[human[j].x][y][human[j].z] == 7)
                                                world[human[j].x][y][human[j].z] = 0;
                                    }
                                    human[j].targeted = -1;
                                    human[j].dead = 1;
                              }
                        }
                        break;
                  }
            }
      }
      beamTick = 8;
   }
//    else if (button == GLUT_MIDDLE_BUTTON)
//       printf("middle button - ");   
//    else
//       printf("right button - ");

//    if (state == GLUT_UP)
//       printf("up - ");
//    else
//       printf("down - ");

//    printf("%d %d\n", x, y);
}

void createHuman(int number, int x, int y, int z) {
      world[x][y][z] = 7;
      world[x][y+1][z] = 3;
      world[x][y+2][z] = 1;
      human[number].x = x;
      human[number].y = y;
      human[number].z = z;
      human[number].targeted = -1;
      human[number].dead = 0;
      human[number].fall = 0;
}


int main(int argc, char** argv)
{
int i, j, k;
	/* initialize the graphics system */
   graphicsInit(&argc, argv);

	/* the first part of this if statement builds a sample */
	/* world which will be used for testing */
	/* DO NOT remove this code. */
	/* Put your code in the else statment below */
	/* The testworld is only guaranteed to work with a world of
		with dimensions of 100,50,100. */
   if (testWorld == 1) {
	/* initialize world to empty */
      for(i=0; i<WORLDX; i++)
         for(j=0; j<WORLDY; j++)
            for(k=0; k<WORLDZ; k++)
               world[i][j][k] = 0;

	/* some sample objects */
	/* build a red platform */
      for(i=0; i<WORLDX; i++) {
         for(j=0; j<WORLDZ; j++) {
            world[i][24][j] = 3;
         }
      }
	/* create some green and blue cubes */
      world[50][25][50] = 1;
      world[49][25][50] = 1;
      world[49][26][50] = 1;
      world[52][25][52] = 2;
      world[52][26][52] = 2;

	/* create user defined colour and draw cube */
      setUserColour(9, 0.7, 0.3, 0.7, 1.0, 0.3, 0.15, 0.3, 1.0);
      world[54][25][50] = 9;


	/* blue box shows xy bounds of the world */
      for(i=0; i<WORLDX-1; i++) {
         world[i][25][0] = 2;
         world[i][25][WORLDZ-1] = 2;
      }
      for(i=0; i<WORLDZ-1; i++) {
         world[0][25][i] = 2;
         world[WORLDX-1][25][i] = 2;
      }

	/* create two sample mobs */
	/* these are animated in the update() function */
      createMob(0, 50.0, 25.0, 52.0, 0.0);
      createMob(1, 50.0, 25.0, 52.0, 0.0);

	/* create sample player */
      createPlayer(0, 52.0, 27.0, 52.0, 0.0);
   } else {
	/* your code to build the world goes here */
      setUserColour(9, 0.7, 0.3, 0.7, 1.0, 0.3, 0.15, 0.3, 1.0);
      setUserColour(10, 0.5, 0.5, 0.5, 1.0, 0.2, 0.2, 0.2, 1.0);
      setUserColour(11, 0.0, 0.5, 0.0, 1.0, 0.0, 0.5, 0.0, 1.0);
      setUserColour(12, 1.0, 1.0, 0.0, 1.0, 0.3, 0.3, 0.15, 1.0);
      /* initialize world to empty */
      for(i=0; i<WORLDX; i++)
         for(j=0; j<WORLDY; j++)
            for(k=0; k<WORLDZ; k++)
               world[i][j][k] = 0;

      /* Build world from ground.ppm */
      FILE * fp;
      fp = fopen("ground.pgm", "r");
      if (fp == NULL) printf("File not found");
      for (int i=0; i<4; i++) fscanf(fp, "%*[^\n]\n", NULL);
      int input;
      for (int x=0; x<100; x++) {
            for (int z=0; z<100; z++) {
                  fscanf(fp, "%d", &input);
                  input /= 30; // scale factor
                  for (int y=input; y >= 0; y--)
                        world[x][y][z] = 9;
            }
      }
      fclose(fp);

      /* create sample humans */
      createHuman(0, 10, 10, 90);
      createHuman(1, 60, 10, 40);
      createHuman(2, 25, 10, 73);
      createHuman(3, 80, 10, 35);
      createHuman(4, 72, 10, 56);
      createHuman(5, 38, 10, 10);
      createHuman(6, 63, 10, 40);
      createHuman(7, 61, 10, 42);

      // init aliens
      srand(time(NULL));   // Initialization, should only be called once.
      for (int i=0; i<ALIEN_MAX; i++) {
            alien[i].state = INIT;
      }
   }


	/* starts the graphics processing loop */
	/* code after this will not run until the program exits */
   glutMainLoop();
   return 0; 
}

