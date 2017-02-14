/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: renachen
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
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

OpenGLMatrix* matrix;
BasicPipelineProgram* pipelineProgram;
GLuint elementBuffer; // for positions
GLuint indexBuffer; // for landIndices
GLuint vao;

// etc.
int numVertices;
//float positions[3][3] = {{0.0, 0.0, -1.0}, {1.0,0.0,-1.0}, {0.0, 1.0, -1.0}};
//float colors[3][4] = {{1.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}};
float* positions;
float* colors;
GLuint* landIndices; // The index to draw to.

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

// Bind buffer, vao and pipelineProgram, upload projection matrix
void bindProgram() {
    GLuint program = pipelineProgram->GetProgramHandle();

    // Bind
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
    pipelineProgram->Bind();
    glBindVertexArray(vao);

    // Upload model and projection matrices
    GLboolean isRowMajor = GL_FALSE;

    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
    float m[16]; // column-major
    matrix->GetMatrix(m);
    glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);

    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    GLint h_projectionViewMatrix = glGetUniformLocation(program, "projectionMatrix");
    float p[16]; // column-major
    matrix->GetMatrix(p);
    glUniformMatrix4fv(h_projectionViewMatrix, 1, isRowMajor, p);

}

void displayFunc()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // set up camera position
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();
    matrix->LookAt(0, 100, 50, 0, 0, 0, 0, 1, 0);// default camera
    matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
    matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
    matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
    matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix->Scale(landScale[0], landScale[1], landScale[2]);
    //matrix->Rotate(0, 0.0, 0.0, 1.0);

    bindProgram();
    // render
    glDrawArrays(GL_POINTS, 0, numVertices);
    //glDrawElements(GL_TRIANGLES, numVertices*2, GL_UNSIGNED_BYTE, (GLvoid*)0);

    glBindVertexArray(0); // unbind vao
    glutSwapBuffers();
}

void idleFunc()
{
    // do some stuff...

    // for example, here, you can save the screenshots to disk (to make the animation)

    // make the screen update
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    GLfloat aspect = (GLfloat) w / (GLfloat) h;
    glViewport(0, 0, w, h);
    // Set up perspective matrix
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->LoadIdentity();
    matrix->Perspective(45.0, 1.0*1280/720, 0.01, 1000);
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
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
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

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
  }
}

// Initialize BasicPipelineProgram
void initPipelineProgram() {
    pipelineProgram = new BasicPipelineProgram();
    pipelineProgram->Init("../openGLHelper-starterCode");
}

// Initialize VAO
void initVAO() {
    // Generate vertex array and bind
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint program = pipelineProgram->GetProgramHandle();

    // Bind shaders
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void* offset = (const void*)0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);

    GLuint loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2);
    offset = (const void*)(numVertices*sizeof(float)*3);
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset);

    glBindVertexArray(0); // Unbind VAO

}

void initVBO() {
    // load buffer
    // glGenBuffers(1, &buffer);
    // glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(colors), NULL, GL_STATIC_DRAW);
    // int positionsArraySize = numVertices*sizeof(float)*3;
    // int colorsArraySize = numVertices*sizeof(float)*4;
    // glBufferData(GL_ARRAY_BUFFER, positionsArraySize + colorsArraySize, NULL, GL_STATIC_DRAW);

    // Element array, for all vertices
    glGenBuffers(1, &elementBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
    //glBufferData(GL_ARRAY_BUFFER, numVertices, NULL, GL_STATIC_DRAW);
    int positionsArraySize = numVertices*sizeof(float)*3;
    int colorsArraySize = numVertices*sizeof(float)*4;
    glBufferData(GL_ARRAY_BUFFER, positionsArraySize + colorsArraySize, NULL, GL_STATIC_DRAW);

    // Put data in buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, positionsArraySize, positions);
    glBufferSubData(GL_ARRAY_BUFFER, positionsArraySize, colorsArraySize, colors);

    // Index array
    // int landIndicesSize = numVertices * 2;
    // glGenBuffers(1, &indexBuffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, landIndicesSize, landIndices, GL_STATIC_DRAW);
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

    // --- [Construct positions and colors arrays] ---
    int img_height = heightmapImage->getHeight();
    int img_width = heightmapImage->getWidth();
    numVertices = img_height * img_width;

    // Malloc positions
    positions = (float*)malloc(numVertices*sizeof(float)*3);
    colors = (float*)malloc(numVertices*sizeof(float)*4);

    // Fill element buffer
    int counter = 0;
    for(int i=0; i<img_height; i++) {
        for(int j=0; j<img_width; j++) {
            float x = i;
            float y = heightmapImage->getPixel(i, j, 0)/256.0;
            float z = -1*j;
            int startPos = 3*(i*img_width + j);
            positions[startPos] = x;
            positions[startPos + 1] = y;
            positions[startPos + 2] = z;
        }
    }

    // Fill indices
    // counter = 0;
    // for(int i=0; i<img_height-1; i++) {
    //     for(int j=0; j<img_width-1; j++) {
    //         // Lower vertex
    //         landIndices[counter] = i*img_width+j;
    //         landIndices[counter] = i*img_width+j+1;
    //         landIndices[counter] = i*img_width+j+2;
    //         //Upper diagonal vertex
    //
    //
    //
    //         // // lower triangle
    //         // landIndices[counter] = i*img_width+j;
    //         // counter++;
    //         // landIndices[counter] = i*img_width+j+1;
    //         // counter++;
    //         // landIndices[counter] = (i+1)*img_width+j+1;
    //         // counter++;
    //         // // upper triangle
    //         // landIndices[counter] = i*img_width+j;
    //         // counter++;
    //         // landIndices[counter] = (i+1)*img_width+j;
    //         // counter++;
    //         // landIndices[counter] = (i+1)*img_width+j+1;
    //         // counter++;
    //     }
    // }

    for(int i=0; i<numVertices*4; i+=4) {
        colors[i] = 1; // R
        colors[i + 1] = 0; // G
        colors[i + 2] = 0; // B
        colors[i + 3] = 1; // alpha
    }


    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // --- [Matrix, VBO, PipelineProgram, and VAO initialization] ---
    matrix = new OpenGLMatrix();
    initVBO();
    initPipelineProgram();
    initVAO();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
    }

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
    free(positions);
    free(colors);
}
