/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: Shreya Bhaumik
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

// color image
int bytesPerPixel;
typedef enum { GRAYSCALE, COLOR } COLOR_STATE;
COLOR_STATE colorState = GRAYSCALE;
// overlay color image
ImageIO * colorImage;
int ARGC;

// GLuint triVertexBuffer, triColorVertexBuffer;
// GLuint triVertexArray;
// int sizeTri;

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;

// readHeightFieldMode0
int imageW, imageH;
float height, scale, red, green, blue, alpha = 1.0;
std::vector<float> pointPosition, pointColor;
std::vector<float> linePosition, lineColor;
std::vector<float> fixedColorMeshPosition, fixedColorMeshColor;
std::vector<float> trianglePosition, triangleColor;
// readHeightFieldMode1
std::vector<float> triPleftPosition, triPrightPosition, triPdownPosition, triPupPosition;
// VBOs and VAOs
GLuint vboPoint, vboLine, vboFixedColorMesh, vboTriangle;
GLuint vaoPoint, vaoLine, vaoFixedColorMesh, vaoTriangle;
GLuint vboTriPleft, vboTriPright, vboTriPdown, vboTriPup;
GLuint vaoMode1;
// display option
typedef enum { POINTS, LINES, TRIANGLES, SMOOTH, MESHANDSOLID } DISPLAY_OPTION;
DISPLAY_OPTION displayOption = POINTS;
// To make the heightfield rotate automatically about y axis
bool autoRot = false; int toggle = -1;
// Screenshot counter
int shotCount = 0;
// To start 300 screenshots
bool takeShots = false;
// For animation
bool animate = false;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  int ww = windowWidth * 2;
  int hh = windowHeight * 2;
  unsigned char * screenshotData = new unsigned char[ww * hh * 3 * 4];
  glReadPixels(0, 0, ww, hh, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(ww, hh, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
  // render some stuff...
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  // matrix.LookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
  int lookAtZ = (imageW < 200)? 0 : (imageW < 500)? 128: (imageW < 700)? 448 : 640;
  matrix.LookAt(128, 128, lookAtZ, 0, 0, 0, 0, 1, 0);


  // For transformations
  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1.0, 0.0, 0.0);
  matrix.Rotate(landRotate[1], 0.0, 1.0, 0.0);
  matrix.Rotate(landRotate[2], 0.0, 0.0, 1.0);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  // ModelView matrix - column major
  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  // Projection matrix - column major
  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);
  
  // bind shader
  pipelineProgram->Bind();

  // set variable

  // To upload MV matrix to GPU
  pipelineProgram->SetModelViewMatrix(m);
  // As also done by
  // GLuint modelViewMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "modelViewMatrix");
  // glUniformMatrix4fv(modelViewMatrix, 1, GL_FALSE, m);

  // To upload P matrix to GPU
  pipelineProgram->SetProjectionMatrix(p);
  // As also done by
  // GLuint projectionMatrix = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "projectionMatrix");
  // glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, p);

  switch(displayOption) 
  {
    case POINTS:
      glBindVertexArray(vaoPoint);
      glDrawArrays(GL_POINTS, 0, pointPosition.size()/3);
    break;

    case LINES:
      glBindVertexArray(vaoLine);
      glDrawArrays(GL_LINES, 0, linePosition.size()/3);
    break;

    case TRIANGLES:
      glBindVertexArray(vaoTriangle);
      glDrawArrays(GL_TRIANGLES, 0, trianglePosition.size()/3);
    break;

    case SMOOTH:
      glBindVertexArray(vaoMode1);
      glDrawArrays(GL_TRIANGLES, 0, trianglePosition.size()/3);
    break;

    case MESHANDSOLID:
      glBindVertexArray(vaoFixedColorMesh);
      glDrawArrays(GL_LINES, 0, fixedColorMeshPosition.size()/3);
      glPolygonOffset(1.0f,1.0f); // Applying glPolygonOffset on GL_TRIANGLES as it works only with polygonal primitives. It won't work on GL_LINES or GL_POINTS.
      glBindVertexArray(vaoTriangle);
      glDrawArrays(GL_TRIANGLES, 0, trianglePosition.size()/3);
    break;
  }

  // glBindVertexArray(triVertexArray);
  // glDrawArrays(GL_TRIANGLES, 0, sizeTri);

  glBindVertexArray(0);
  glutSwapBuffers();
}

void idleFunc()
{
  // do some stuff... 
  // To make the heightfield rotate automatically about y axis 
  if (autoRot)
    landRotate[1] += 0.7f;
  if (toggle == -1)
    autoRot = false;
  // To animate
  if(animate || takeShots)
  {
    GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
    // if (shotCount == 0) displayOption = MESHANDSOLID;
    if (shotCount >300) {displayOption = MESHANDSOLID; glUniform1i(loc,0); shotCount = 0; animate = false; takeShots = false; autoRot = false;}
    if (shotCount <= 300)
    {
      if (shotCount <= 30) // First 30 screenshots in POINT mode
      {
        autoRot = true;
        displayOption = POINTS;
        glUniform1i(loc,0);
      }
      else if (shotCount <= 60) // Next 30 screenshots in LINE mode
      {
        autoRot = true;
        displayOption = LINES;
        glUniform1i(loc,0);
      }
      else if (shotCount <= 140) // Next 80 screenshots in TRIANGLE mode
      {
        autoRot = false;
        displayOption = TRIANGLES;
        glUniform1i(loc,0);
        if (shotCount <= 100) {landTranslate[0] += 0.4f; landTranslate[1] += 0.4f; landTranslate[2] -= 0.7f;}
        else {landTranslate[0] -= 0.4f; landTranslate[1] -= 0.4f; landTranslate[2] += 0.7f;}
      }
      else if (shotCount <= 220) // Next 80 screenshots in SMOOTH mode
      {
        autoRot = false;
        displayOption = SMOOTH;
        glUniform1i(loc,1);
        if (shotCount <= 180) {landTranslate[0] -= 0.4f; landTranslate[1] += 0.4f; landTranslate[2] += 0.7f;}
        else {landTranslate[0] += 0.4f; landTranslate[1] -= 0.4f; landTranslate[2] -= 0.7f;}
      }
      else // Last 80 screenshots in MESHANDSOLID mode
      {
        displayOption = MESHANDSOLID;
        glUniform1i(loc,0);
        autoRot = true;
        if (shotCount <= 260) {landScale[0] += 0.02f; landScale[1] += 0.02f; landScale[2] += 0.02f;}
        else {landScale[0] -= 0.02f; landScale[1] -= 0.02f; landScale[2] -= 0.02f;}
      }
      shotCount++;
    }
  }
  // To save the 300 screenshots to disk
  if ((((shotCount-2) >= 0) && ((shotCount-2) < 300)) && (takeShots))
  {
    char filenum[4];
    sprintf(filenum, "%03d", (shotCount-2));
    std::string filename("Animations/color/" + std::string(filenum) + ".jpg");
    saveScreenshot(filename.c_str());
  }
  else if ((shotCount-2) >= 300)
    takeShots = false;

  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // matrix.Perspective(54.0f, (float)w / (float)h, 0.01f, 100.0f);
  matrix.Perspective(60.0f, (float)w / (float)h, 0.01f, 2000.0f); // Since 60 is the FOV of human eye
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    // CTRL is not working on macOS Catalina 10.15.3
    // case GLUT_ACTIVE_CTRL:
    //   controlState = TRANSLATE;
    // break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    break;

    case '1':
      displayOption = POINTS;
      // mode
      glUniform1i(loc, 0);
    break;

    case '2':
      displayOption = LINES;
      // mode
      glUniform1i(loc, 0);
    break;

    case '3':
      displayOption = TRIANGLES;
      // mode
      glUniform1i(loc, 0);
    break;

    case '4':
      displayOption = SMOOTH;
      // mode
      glUniform1i(loc, 1);
    break;

    case '5':
      displayOption = MESHANDSOLID;
      // mode
      glUniform1i(loc, 0);
    break;

    case 't': // CTRL is not working on macOS Catalina 10.15.3 so we use t for translate
      controlState = TRANSLATE;
    break;

    case 'r': // To make the heightfield rotate automatically about y axis
      autoRot = true;
      toggle = -toggle;
    break;

    case 'a': // To run animation
      animate = true;
    break;

    case 's': // To take 300 screenshots
      takeShots = true;
    break;
  }
}

void readColorOverlayImage(int i, int j)
{
  red = colorImage->getPixel(i, j, 0) / 255.0;
  green = colorImage->getPixel(i, j, 1) / 255.0;
  blue = colorImage->getPixel(i, j, 2) / 255.0;
}

void computeColorPixel(int i, int j)  // To compute the height from color and also populate color from color-image pixel
{
  // Colorimetric (perceptual luminance-preserving) conversion to grayscale following the steps in https://en.wikipedia.org/wiki/Grayscale#Converting_color_to_grayscale
  red = heightmapImage->getPixel(i, j, 0) / 255.0;
  green = heightmapImage->getPixel(i, j, 1) / 255.0;
  blue = heightmapImage->getPixel(i, j, 2) / 255.0;

  float Clinear, Csrgb;
  // Relative luminance
  Clinear = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
  // 
  if (Clinear <= 0.0031308)
    Csrgb = 12.92 * Clinear;
  else
    Csrgb = 1.055 * powf(Clinear,0.41666666666) - 0.055;
  height = Csrgb * 255.0 * scale;
}

void readHeightFieldMode0()
{
  imageW = heightmapImage->getWidth();
  imageH = heightmapImage->getHeight();

  // To put the height field's center at the world origin
  int imageWC, imageHC;
  imageWC = imageW/2;
  imageHC = imageH/2;

  scale = (imageW < 200)? 0.11 : (imageW < 500)? 0.16: (imageW < 700)? 0.23 : 0.34;

  for (int i = -imageWC; i < (imageWC-1); i++)
    for (int j = -imageHC; j < (imageHC-1); j++)
    {

      // For Point mode
      if (colorState == GRAYSCALE)  // If color of image is in grayscale
      {
        height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
        if (ARGC == 3)  // If an overlay color image is provided to get color to put on grayscale heightfield
          readColorOverlayImage(i+imageWC, j+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else  // If it is a color image
        computeColorPixel(i+imageWC, j+imageHC);
      pointPosition.push_back((float)i);
      pointPosition.push_back(height);
      pointPosition.push_back(-(float)j);
      pointColor.push_back(red);
      pointColor.push_back(green);
      pointColor.push_back(blue);
      pointColor.push_back(alpha);

      // For Line/wireframe mode
      if(i < (imageWC-1)-1) // For horizontal lines
      {
        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
          if (ARGC == 3)
            readColorOverlayImage(i+imageWC, j+imageHC);
          else
            red = green = blue = height / 255.0;
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, j+imageHC);
        linePosition.push_back((float)i);
        linePosition.push_back(height);
        linePosition.push_back(-(float)j);
        lineColor.push_back(red);
        lineColor.push_back(green);
        lineColor.push_back(blue);
        lineColor.push_back(alpha);

        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0);
          if (ARGC == 3)
            readColorOverlayImage((i+1)+imageWC, j+imageHC);
          else
            red = green = blue = height / 255.0;
          height *= scale;
        }
        else
          computeColorPixel((i+1)+imageWC, j+imageHC);
        linePosition.push_back((float)(i+1));
        linePosition.push_back(height);
        linePosition.push_back(-(float)j);
        lineColor.push_back(red);
        lineColor.push_back(green);
        lineColor.push_back(blue);
        lineColor.push_back(alpha);
      }
      if(j < (imageHC-1)-1) // For vertical lines
      {
        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
          if (ARGC == 3)
            readColorOverlayImage(i+imageWC, j+imageHC);
          else
            red = green = blue = height / 255.0;
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, j+imageHC);
        linePosition.push_back((float)i);
        linePosition.push_back(height);
        linePosition.push_back(-(float)j);
        lineColor.push_back(red);
        lineColor.push_back(green);
        lineColor.push_back(blue);
        lineColor.push_back(alpha);

        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0);
          if (ARGC == 3)
            readColorOverlayImage(i+imageWC, (j+1)+imageHC);
          else
            red = green = blue = height / 255.0;
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, (j+1)+imageHC);
        linePosition.push_back((float)i);
        linePosition.push_back(height);
        linePosition.push_back(-(float)(j+1));
        lineColor.push_back(red);
        lineColor.push_back(green);
        lineColor.push_back(blue);
        lineColor.push_back(alpha);
      }

        // For fixed color mesh of meshandsolid mode
      if(i < (imageWC-1)-1) // For horizontal lines
      {
        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, j+imageHC);
        red = 0.1;
        green = 0.0;
        blue = 0.2;
        fixedColorMeshPosition.push_back((float)i);
        fixedColorMeshPosition.push_back(height);
        fixedColorMeshPosition.push_back(-(float)j);
        fixedColorMeshColor.push_back(red);
        fixedColorMeshColor.push_back(green);
        fixedColorMeshColor.push_back(blue);
        fixedColorMeshColor.push_back(alpha);

        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0);
          height *= scale;
        }
        else
          computeColorPixel((i+1)+imageWC, j+imageHC);
        red = 0.1;
        green = 0.0;
        blue = 0.2;
        fixedColorMeshPosition.push_back((float)(i+1));
        fixedColorMeshPosition.push_back(height);
        fixedColorMeshPosition.push_back(-(float)j);
        fixedColorMeshColor.push_back(red);
        fixedColorMeshColor.push_back(green);
        fixedColorMeshColor.push_back(blue);
        fixedColorMeshColor.push_back(alpha);
      }
      if(j < (imageHC-1)-1) // For vertical lines
      {
        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, j+imageHC);
        red = 0.1;
        green = 0.0;
        blue = 0.2;
        fixedColorMeshPosition.push_back((float)i);
        fixedColorMeshPosition.push_back(height);
        fixedColorMeshPosition.push_back(-(float)j);
        fixedColorMeshColor.push_back(red);
        fixedColorMeshColor.push_back(green);
        fixedColorMeshColor.push_back(blue);
        fixedColorMeshColor.push_back(alpha);

        if (colorState == GRAYSCALE)
        {
          height = heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0);
          height *= scale;
        }
        else
          computeColorPixel(i+imageWC, (j+1)+imageHC);
        red = 0.1;
        green = 0.0;
        blue = 0.2;
        fixedColorMeshPosition.push_back((float)i);
        fixedColorMeshPosition.push_back(height);
        fixedColorMeshPosition.push_back(-(float)(j+1));
        fixedColorMeshColor.push_back(red);
        fixedColorMeshColor.push_back(green);
        fixedColorMeshColor.push_back(blue);
        fixedColorMeshColor.push_back(alpha);
      }
    }

  // For Triangle mode
  for (int i = -imageWC; i < (imageWC-1); i++)
    for (int j = -imageHC; j < (imageHC-1); j++)
    {
      //    /|
      //   / |
      //  /  |
      //  ----
      
      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage(i+imageWC, j+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
          computeColorPixel(i+imageWC, j+imageHC);
      trianglePosition.push_back((float)i);
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)j);
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);

      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage((i+1)+imageWC, j+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
        computeColorPixel((i+1)+imageWC, j+imageHC);
      trianglePosition.push_back((float)(i+1));
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)j);
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);

      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage((i+1)+imageWC, (j+1)+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
        computeColorPixel((i+1)+imageWC, (j+1)+imageHC);
      trianglePosition.push_back((float)(i+1));
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)(j+1));
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);
      
      //  ----
      //  |  /
      //  | /
      //  |/

      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage(i+imageWC, j+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
        computeColorPixel(i+imageWC, j+imageHC);
      trianglePosition.push_back((float)i);
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)j);
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);

      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage((i+1)+imageWC, (j+1)+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
        computeColorPixel((i+1)+imageWC, (j+1)+imageHC);
      trianglePosition.push_back((float)(i+1));
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)(j+1));
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);

      if (colorState == GRAYSCALE)
      {
        height = heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0);
        if (ARGC == 3)
          readColorOverlayImage(i+imageWC, (j+1)+imageHC);
        else
          red = green = blue = height / 255.0;
        height *= scale;
      }
      else
        computeColorPixel(i+imageWC, (j+1)+imageHC);
      trianglePosition.push_back((float)i);
      trianglePosition.push_back(height);
      trianglePosition.push_back(-(float)(j+1));
      triangleColor.push_back(red);
      triangleColor.push_back(green);
      triangleColor.push_back(blue);
      triangleColor.push_back(alpha);
    }
}

void readHeightFieldMode1() // To read height field data into neighbours of point
{
  imageW = heightmapImage->getWidth();
  imageH = heightmapImage->getHeight();

  // To put the height field's center at the world origin
  int imageWC, imageHC;
  imageWC = imageW/2;
  imageHC = imageH/2;

  scale = (imageW < 200)? 0.11 : (imageW < 500)? 0.16: (imageW < 700)? 0.23 : 0.34;

  for (int i = -imageWC; i < (imageWC-1); i++)
    for (int j = -imageHC; j < (imageHC-1); j++)
    {
      //    /|
      //   / |
      //  /  |
      //  ----
      // For p(i,j)
      // Pleft - (i-1,j)
      if (i == -imageWC) // if p(0,0) or p(0,j!=0) then Pleft = Pcenter
        height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i-1)+imageWC, j+imageHC, 0);
      triPleftPosition.push_back((float)(i-1));
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)j);
      // Pright - (i+1,j)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0); // There's always going to be a right
      triPrightPosition.push_back((float)(i+1));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)j);
      // Pdown - (i,j-1)
      if (j == -imageHC) /// if p(0,0) or p(i!=0,0) then Pdown = Pcenter
        height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel(i+imageWC, (j-1)+imageHC, 0);
      triPdownPosition.push_back((float)i);
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)(j-1));
      // Pup - (i,j+1)
      height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0); // There's always going to be an up
      triPupPosition.push_back((float)i);
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+1));

      // For p(i+1,j)
      // Pleft - (i,j)
      height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0); // There's always going to be a left
      triPleftPosition.push_back((float)i);
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)j);
      // Pright - (i+2,j)
      if (i == imageWC-2) // if p(imageW,0) or p(imageW,j!=0) then Pright=Pcenter
        height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+2)+imageWC, j+imageHC, 0);
      triPrightPosition.push_back((float)(i+2));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)j);
      // Pdown - (i+1,j-1)
      if (j == -imageHC)
        height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j-1)+imageHC, 0);
      triPdownPosition.push_back((float)(i+1));
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)(j-1));
      // Pup - (i+1,j+1)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0); // There's always going to be an up
      triPupPosition.push_back((float)(i+1));
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+1));

      // For p(i+1,j+1)
      // Pleft - (i,j+1)
      height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0); // There's always going to be a left
      triPleftPosition.push_back((float)i);
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)(j+1));
      // Pright - (i+2,j+1)
      if (i == imageWC-2) // if p(imageW,imageH) or p(imageW,j) then Pright=Pcenter
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+2)+imageWC, (j+1)+imageHC, 0);
      triPrightPosition.push_back((float)(i+2));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)(j+1));
      // Pdown - (i+1,j)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0); // There's always going to be a down
      triPdownPosition.push_back((float)(i+1));
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)j);
      // Pup - (i+1,j+2)
      if (j == imageHC-2) // if p(imageW,imageH) or p(i,imageH) then Pup=Pcenter
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+2)+imageHC, 0);
      triPupPosition.push_back((float)(i+1));
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+2));
      
      //  ----
      //  |  /
      //  | /
      //  |/

      // For p(i,j)
      // Pleft - (i-1,j)
      if (i == -imageWC) // if p(0,j) or p(0,j!=0)
        height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i-1)+imageWC, j+imageHC, 0);
      triPleftPosition.push_back((float)(i-1));
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)j);
      // Pright - (i+1,j)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0); // There's always going to be a right
      triPrightPosition.push_back((float)(i+1));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)j);
      // Pdown - (i,j-1)
      if (j == -imageHC) // if p(0,0) or p(i!=0,0) then Pdown=Pcenter
        height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel(i+imageWC, (j-1)+imageHC, 0);
      triPdownPosition.push_back((float)i);
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)(j-1));
      // Pup - (i,j+1)
      height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0); // There's always going to be an up
      triPupPosition.push_back((float)i);
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+1));

      // For p(i+1,j+1)
      // Pleft - (i,j+1)
      height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0); // There's always going to be a left
      triPleftPosition.push_back((float)i);
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)(j+1));
      // Pright - (i+2,j+1)
      if (i == imageWC-2) // if p(imageW,imageH) or p(imageW,j) then Pright=Pcenter
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+2)+imageWC, (j+1)+imageHC, 0);
      triPrightPosition.push_back((float)(i+2));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)(j+1));
      // Pdown - (i+1,j)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, j+imageHC, 0); // There's always going to be a down
      triPdownPosition.push_back((float)(i+1));
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)j);
      // Pup - (i+1,j+2)
      if (j == imageHC-2) // if p(imageW,imageH) or p(i,imageH) then Pup=Pcenter
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+2)+imageHC, 0);
      triPupPosition.push_back((float)(i+1));
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+2));

      // For p(i,j+1)
      // Pleft - (i-1,j+1)
      if (i == -imageWC) // if p(0,j)
        height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel((i-1)+imageWC, (j+1)+imageHC, 0);
      triPleftPosition.push_back((float)(i-1));
      triPleftPosition.push_back(height);
      triPleftPosition.push_back(-(float)(j+1));
      // Pright - (i+1,j+1)
      height = scale * heightmapImage->getPixel((i+1)+imageWC, (j+1)+imageHC, 0); // There's always going to be a right
      triPrightPosition.push_back((float)(i+1));
      triPrightPosition.push_back(height);
      triPrightPosition.push_back(-(float)(j+1));
      // Pdown - (i,j)
      height = scale * heightmapImage->getPixel(i+imageWC, j+imageHC, 0); // There's always going to be a down
      triPdownPosition.push_back((float)i);
      triPdownPosition.push_back(height);
      triPdownPosition.push_back(-(float)j);
      // Pup - (i,j+2)
      if (j == imageHC-2) // if p(0,imageH) or p(i!=0,imageH) then Pup=Pcenter
        height = scale * heightmapImage->getPixel(i+imageWC, (j+1)+imageHC, 0);
      else
        height = scale * heightmapImage->getPixel(i+imageWC, (j+2)+imageHC, 0);
      triPupPosition.push_back((float)i);
      triPupPosition.push_back(height);
      triPupPosition.push_back(-(float)(j+2));
    }
}


void initVBO_VAO_mode0()
{
  // For points
  glGenBuffers(1, &vboPoint); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * pointPosition.size() + sizeof(float) * pointColor.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * pointPosition.size(), (float*)(pointPosition.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * pointPosition.size(), sizeof(float) * pointColor.size(), (float*)(pointColor.data()));
  
  glGenVertexArrays(1, &vaoPoint);
  glBindVertexArray(vaoPoint); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
  // get location index of the "position" shader variable
  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  const void * offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float)*pointPosition.size());
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "color" attribute data
  glBindVertexArray(0); // unbind the VAO


  // For lines
  glGenBuffers(1, &vboLine); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboLine);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * linePosition.size() + sizeof(float) * lineColor.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * linePosition.size(), (float*)(linePosition.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * linePosition.size(), sizeof(float) * lineColor.size(), (float*)(lineColor.data()));
  
  glGenVertexArrays(1, &vaoLine);
  glBindVertexArray(vaoLine); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboLine);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float)*linePosition.size());
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "color" attribute data
  glBindVertexArray(0); // unbind the VAO

  // For fixedColorMesh
  glGenBuffers(1, &vboFixedColorMesh); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboFixedColorMesh);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * fixedColorMeshPosition.size() + sizeof(float) * fixedColorMeshColor.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * fixedColorMeshPosition.size(), (float*)(fixedColorMeshPosition.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * fixedColorMeshPosition.size(), sizeof(float) * fixedColorMeshColor.size(), (float*)(fixedColorMeshColor.data()));
  
  glGenVertexArrays(1, &vaoFixedColorMesh);
  glBindVertexArray(vaoFixedColorMesh); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboFixedColorMesh);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float)*fixedColorMeshPosition.size());
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "color" attribute data
  glBindVertexArray(0); // unbind the VAO

  // For triangles
  glGenBuffers(1, &vboTriangle); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * trianglePosition.size() + sizeof(float) * triangleColor.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * trianglePosition.size(), (float*)(trianglePosition.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * trianglePosition.size(), sizeof(float) * triangleColor.size(), (float*)(triangleColor.data()));
  
  glGenVertexArrays(1, &vaoTriangle);
  glBindVertexArray(vaoTriangle); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float)*trianglePosition.size());
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "color" attribute data
  glBindVertexArray(0); // unbind the VAO
}

void initVBO_VAO_mode1()
{
  // For Pleft
  glGenBuffers(1, &vboTriPleft); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriPleft);  // bind the VBO buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * triPleftPosition.size(), (float*)(triPleftPosition.data()), GL_STATIC_DRAW);

  glGenVertexArrays(1, &vaoMode1);
  glBindVertexArray(vaoMode1); // bind the VAO
  glBindBuffer(GL_ARRAY_BUFFER, vboTriPleft);
  // get location index of the "PleftPosition" shader variable
  GLuint loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "PleftPosition");
  glEnableVertexAttribArray(loc); // enable the "PleftPosition" attribute
  const void * offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "PleftPosition" attribute data

  // For Pright
  glGenBuffers(1, &vboTriPright); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriPright);  // bind the VBO buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * triPrightPosition.size(), (float*)(triPrightPosition.data()), GL_STATIC_DRAW);
  // get location index of the "PrightPosition" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "PrightPosition");
  glEnableVertexAttribArray(loc); // enable the "PrightPosition" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "PrightPosition" attribute data
  
  // For Pdown
  glGenBuffers(1, &vboTriPdown); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriPdown);  // bind the VBO buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * triPdownPosition.size(), (float*)(triPdownPosition.data()), GL_STATIC_DRAW);
  // get location index of the "PdownPosition" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "PdownPosition");
  glEnableVertexAttribArray(loc); // enable the "PdownPosition" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "PdownPosition" attribute data

  // For Pup
  glGenBuffers(1, &vboTriPup); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriPup);  // bind the VBO buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * triPupPosition.size(), (float*)(triPupPosition.data()), GL_STATIC_DRAW);
  // get location index of the "PupPosition" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "PupPosition");
  glEnableVertexAttribArray(loc); // enable the "PupPosition" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "PupPosition" attribute data

  // For triangles
  glGenBuffers(1, &vboTriangle); // get handle on VBO buffer
  glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);  // bind the VBO buffer
  // glBufferData() can allocate memory but glBufferSubData() cannot
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * trianglePosition.size() + sizeof(float) * triangleColor.size(), NULL, GL_STATIC_DRAW); // to allocate memory
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * trianglePosition.size(), (float*)(trianglePosition.data()));
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * trianglePosition.size(), sizeof(float) * triangleColor.size(), (float*)(triangleColor.data()));
  
  // get location index of the "position" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  glEnableVertexAttribArray(loc); // enable the "position" attribute
  offset = (const void*) 0;
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "position" attribute data
  // get location index of the "color" shader variable
  loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  glEnableVertexAttribArray(loc); // enable the "color" attribute
  offset = (const void*) (sizeof(float)*trianglePosition.size());
  glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, offset); // set the layout of the "color" attribute data
  glBindVertexArray(0); // unbind the VAO
}



void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
  bytesPerPixel = heightmapImage->getBytesPerPixel();
  if (bytesPerPixel == 1)
    colorState = GRAYSCALE;
  else
    colorState = COLOR; // To handle color images

  if (argc == 3) // If color overlay image provided
  {
    colorImage = new ImageIO();
    if (colorImage->loadJPEG(argv[2]) != ImageIO::OK)
    {
      cout << "Error reading image " << argv[1] << "." << endl;
      exit(EXIT_FAILURE);
    }
  }

  // glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClearColor(0.08f, 0.0f, 0.08f, 0.0f);

  // modify the following code accordingly
  // glm::vec3 triangle[3] = {
  //   glm::vec3(0, 0, 0), 
  //   glm::vec3(0, 1, 0),
  //   glm::vec3(1, 0, 0)
  // };

  // glm::vec4 color[3] = {
  //   {0, 0, 1, 1},
  //   {1, 0, 0, 1},
  //   {0, 1, 0, 1},
  // };
  
  readHeightFieldMode0();

  readHeightFieldMode1();

  // glGenBuffers(1, &triVertexBuffer);
  // glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 3, triangle,
  //              GL_STATIC_DRAW);

  // glGenBuffers(1, &triColorVertexBuffer);
  // glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 3, color, GL_STATIC_DRAW);

  pipelineProgram = new BasicPipelineProgram;
  int ret = pipelineProgram->Init(shaderBasePath);
  if (ret != 0) abort();

  pipelineProgram->Bind();

  initVBO_VAO_mode0();

  initVBO_VAO_mode1();

  // glGenVertexArrays(1, &triVertexArray);
  // glBindVertexArray(triVertexArray);
  // glBindBuffer(GL_ARRAY_BUFFER, triVertexBuffer);

  // GLuint loc =
  //     glGetAttribLocation(pipelineProgram->GetProgramHandle(), "position");
  // glEnableVertexAttribArray(loc);
  // glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  // glBindBuffer(GL_ARRAY_BUFFER, triColorVertexBuffer);
  // loc = glGetAttribLocation(pipelineProgram->GetProgramHandle(), "color");
  // glEnableVertexAttribArray(loc);
  // glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (const void *)0);

  glEnable(GL_DEPTH_TEST);

  // sizeTri = 3;

  std::cout << "GL error: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (!((argc != 3) ^ (argc != 2)))
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    cout << "or" << endl;
    cout << "You can additionally provide color image to color the heightmap." << endl;
    cout << "usage: ./hw1 <heightmap file> <color image file>" << endl;
    exit(EXIT_FAILURE);
  }
  ARGC = argc;

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}


