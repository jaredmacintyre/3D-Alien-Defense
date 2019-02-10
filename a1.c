
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

struct timeval oldTime;
int updateTime = -1;

float oldMvt[3] = {0.0, 0.0, 0.0}; // old movement
int beamTick = 0; // beam tick
int tick = 0; // gravity tick
int first = 1;
float accel = 0.0;
int direction = ' ';

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
         GLfloat green[] = {0.0, 0.5, 0.0,0.5};
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
            // Small Map
            float scale = screenHeight*screenWidth*0.0002;
            float x1 = screenWidth/2 + scale*1.3;
            float y1 = screenHeight/2 - scale*1.3;
            float x2 = screenWidth/2 - scale*1.3;
            float y2 = screenHeight/2 + scale*1.3;
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
                        // if (oldMvt[i] == 0.0 || newD != direction) {
                        //       newMvt[i] = (newVP[i] - oldVP[i]) * 0.2;
                        // }
                        // else {
                        //       accel *= ACCEL_RATE;
                        //       newMvt[i] = (newVP[i] - oldVP[i]) * accel;
                        // }
                              
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

      // gravity
      if (tick < 15) tick++;
      else tick = 0;
      if (tick != 0) return;

      for(int x=0; x<WORLDX; x++) {
         for(int y=0; y<WORLDY; y++) {
            for(int z=0; z<WORLDZ; z++) {
                  if (world[x][y][z] == 7 && world[x][y-1][z] != 9) {
                        world[x][y-1][z] = 7;
                        world[x][y][z] = 3;
                        world[x][y+1][z] = 1;
                        world[x][y+2][z] = 0;
                  }
            }
         }
      }

      // beam
      if (beamTick > 0) {
            beamTick--;
            if (beamTick == 0) {
                  for (int i=1; i<=8; i++)
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
      for (int i=0; i<8; i++) {
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
      for (int i=1; i<=8; i++) {
            float end[3];
            getTubeEnd(i, &end[0], &end[1], &end[2]);
            if (world[(int)end[0]][(int)end[1]][(int)end[2]] == 1 || world[(int)end[0]][(int)end[1]][(int)end[2]] == 3 || world[(int)end[0]][(int)end[1]][(int)end[2]] == 7) {
                  if (state == GLUT_UP) printf("Beam Collision with Human\n");
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
      createHuman(0, 10, 40, 90);
      createHuman(1, 60, 46, 40);
      createHuman(2, 25, 44, 73);
      createHuman(3, 80, 40, 35);
      createHuman(4, 72, 48, 56);
      createHuman(5, 38, 36, 10);
   }


	/* starts the graphics processing loop */
	/* code after this will not run until the program exits */
   glutMainLoop();
   return 0; 
}

