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

typedef enum { POINTS, WIRE, MESH} RENDER_STATE;
RENDER_STATE renderState = MESH;

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
GLuint indexBuffer; // for meshIndicies
GLuint lineIndexBuffer; // for wireframe
GLuint vao;

int numVertices;
int img_height;
int img_width;
int polyCount; // number of triangles
int numQuadEdges; // number of lines if mesh divided into quads
//float positions[3][3] = {{0.0, 0.0, -1.0}, {1.0,0.0,-1.0}, {0.0, 1.0, -1.0}};
//float colors[3][4] = {{1.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}};


// Position, color, index arrays
float* positions;
float* colors;
float* normals;
GLuint* meshIndices; // The index to draw to for mesh.
GLuint* lineIndices; // The index to draw to for wireframe.

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

    // Bind --- bind element array in displayFunc
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
    pipelineProgram->Bind();

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
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);

    // Upload light direction vector
    float view[16];
    openGLMatrix->GetMatrix(view); // read the view matrix
    GLint h_viewLightDirection = glGetUniformLocation(program, "viewLightDirection");
    float lightDirection[3] = { 0, 1, 0 }; // position of the Sun



}

void displayFunc()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // set up camera position
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();
    matrix->LookAt(heightmapImage->getWidth()/2, 200, 400, 0, 0, 0, 0, 1, 0);// default camera
    matrix->Translate(-1*img_width/2, 0, img_height/2);
    matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
    matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
    matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
    matrix->Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
    matrix->Scale(landScale[0], landScale[1], landScale[2]);


    bindProgram();
    // render points, wireframe, or mesh based on key pressed
    switch(renderState) {
        case POINTS:
            glDrawArrays(GL_POINTS, 0, numVertices);
        break;

        case WIRE:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
            glDrawElements(GL_LINES, numQuadEdges * 2 * sizeof(GLuint), GL_UNSIGNED_INT, (GLvoid*)0);
        break;

        case MESH:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glDrawElements(GL_TRIANGLES, polyCount * 3 * sizeof(GLuint), GL_UNSIGNED_INT, (GLvoid*)0);
        break;
    }

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

    case 'q':
        cout << "Render points" << endl;
        renderState = POINTS;
    break;

    case 'w':
        cout << "Render wireframe" << endl;
        renderState = WIRE;
    break;

    case 'e':
        cout << "Render mesh" << endl;
        renderState = MESH;
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

    // GLuint loc2 = glGetAttribLocation(program, "color");
    // glEnableVertexAttribArray(loc2);
    // offset = (const void*)(numVertices*sizeof(float)*3);
    // glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset);

    // get location index of the "normal" shader variable
    GLuint loc3 = glGetAttribLocation(program, "normal");
    glEnableVertexAttribArray(loc3); // enable the "normal" attribute
    const void * offset = (const void*) sizeof(positions);
    GLsizei stride = 0;
    GLboolean normalized = GL_FALSE;
    // set the layout of the “normal” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

    glBindVertexArray(0); // Unbind VAO

}

void initVBO() {
    // Load buffer for element array, for all vertices
    glGenBuffers(1, &elementBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, elementBuffer);
    int positionsArraySize = numVertices*sizeof(float)*3;
    int colorsArraySize = numVertices*sizeof(float)*4;
    glBufferData(GL_ARRAY_BUFFER, positionsArraySize + colorsArraySize, NULL, GL_STATIC_DRAW);
    // Put data in buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, positionsArraySize, positions);
    glBufferSubData(GL_ARRAY_BUFFER, positionsArraySize, colorsArraySize, colors);

    // Load buffer for mesh index array
    int meshIndicesSize = polyCount * 3 * sizeof(GLuint);
    cout << meshIndicesSize << endl;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshIndicesSize, meshIndices, GL_STATIC_DRAW);

    // Load buffer for line index array
    int lineIndicesSize = numQuadEdges * 2 * sizeof(GLuint);
    glGenBuffers(1, &lineIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, lineIndicesSize, lineIndices, GL_STATIC_DRAW);
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
    img_height = heightmapImage->getHeight();
    img_width = heightmapImage->getWidth();
    numVertices = img_height * img_width;
    polyCount = (img_height-1)*(img_width-1)*2;
    numQuadEdges = (img_width-1)*img_height + img_width * (img_height-1);

    // Malloc arrays
    positions = (float*)malloc(numVertices*sizeof(float)*3);
    colors = (float*)malloc(numVertices*sizeof(float)*4);
    normals = (float*)malloc(numVertices*sizeof(float)*3);
    meshIndices = (unsigned int*)malloc(polyCount*sizeof(unsigned int)*3);
    lineIndices = (unsigned int*)malloc(numQuadEdges*sizeof(unsigned int)*2);

    // Fill positions
    for(int i=0; i<img_height; i++) {
        for(int j=0; j<img_width; j++) {
            float x = i;
            float y = heightmapImage->getPixel(i, j, 0)/50.0;
            float z = -1*j;
            int startPos = 3*(i*img_width + j);
            positions[startPos] = x;
            positions[startPos + 1] = y;
            positions[startPos + 2] = z;
        }
    }

    // Fill mesh indices in a zigzag pattern
    int counter = 0;
    for(int i=0; i<img_height-1; i++) {
        for(int j=0; j<img_width-1; j++) {
            // Upper triangle
            // Lower vertex
            meshIndices[counter] = i*img_width+j;
            counter++;
            // Upper vertex
            meshIndices[counter] = (i+1)*img_width+j;
            counter++;
            // Upper vertex diagonal
            meshIndices[counter] = (i+1)*img_width+(j+1);
            counter++;

            // Lower triangle
            // Lower vertex
            meshIndices[counter] = i*img_width+j;
            counter++;
            // Next vertex
            meshIndices[counter] = i*img_width+j+1;
            counter++;
            // Upper vertex diagonal
            meshIndices[counter] = (i+1)*img_width+(j+1);
            counter++;
        }
    }

    // Fill line indices
    counter = 0;
    for(int i=0; i<img_height-1; i++) {
        for(int j=0; j<img_width-1; j++) {
            // Bottom line
            lineIndices[counter] = i*img_width+j;
            counter++;
            lineIndices[counter] = i*img_width+j+1;
            counter++;
            // Right line
            lineIndices[counter] = i*img_width+j+1;
            counter++;
            lineIndices[counter] = (i+1)*img_width+j+1;
            counter++;
            // if at left edge, then keep left line
            if(j-1 < 0) {
                lineIndices[counter] = i*img_width+j;
                counter++;
                lineIndices[counter] = (i+1)*img_width+j;
                counter++;
            }
            // if at top, then draw the top lineIndices
            if(i+1 > img_height-2) {
                lineIndices[counter] = (i+1)*img_width+j;
                counter++;
                lineIndices[counter] = (i+1)*img_width+j;
                counter++;
            }
        }
    }

    // compute normals
    for(int i=1; i<img_height-1; i++) {
        for(int j=1; j<img_width-1; j++) {
            // current vertex
            float x0 = i;
            float y0 = heightmapImage->getPixel(i,j);
            float z0 = j;
            // top vertex
            float x1 = i;
            float y1 = heightmapImage->getPixel(i, j+1);
            float z1 = j+1;
            // left vertex
            float x2 = i-1;
            float y2 = heightmapImage->getPixel(i-1, j);
            float z2 = j;
            // bottom vertex
            float x3 = i;
            float y3 = heightmapImage->getPixel(i, j-1);
            float z3 = j-1;
            // right vertex
            float x4 = i+1;
            float y4 = heightmapImage->getPixel(i+1, j);
            float z4 = j;
            // tangent to top vertex
            float topTangentX = x1 - x0;
            float topTangentY = y1 - y0;
            float topTangentZ = z1 - z0;
            // tangent to left vertex
            float leftTangentX = x2 - x0;
            float leftTangentY = y2 - y0;
            float leftTangentZ = z2 - z0;
            // tangent to bottom vertex
            float bottomTangentX = x3 - x0;
            float bottomTangentY = y3 - y0;
            float bottomTangentZ = z3 - z0;
            // tangent to right vertex
            float rightTangentX = x4 - x0;
            float rightTangentY = y4 - y0;
            float rightTangentZ = z4 - z0;
            // cross product top and left
            float topNormalX = (topTangentY * leftTangentZ) - (topTangentZ * leftTangentY);
            float topNormalY = (topTangentZ * leftTangentX) - (topTangentX * leftTangentZ);
            float topNormalZ = (topTangentX * leftTangentY) - (topTangentY * leftTangentX);
            // cross product left and bottom
            float leftNormalX = (leftTangentY * bottomTangentZ) - (leftTangentZ * bottomTangentY);
            float leftNormalY = (leftTangentZ * bottomTangentX) - (leftTangentX * bottomTangentZ);
            float leftNormalZ = (leftTangentX * bottomTangentY) - (leftTangentY * bottomTangentX);
            // cross product bottom and right
            float bottomNormalX = (bottomTangentY * rightTangentZ) - (bottomTangentZ * rightTangentY);
            float bottomNormalY = (bottomTangentZ * rightTangentX) - (bottomTangentX * rightTangentZ);
            float bottomNormalZ = (bottomTangentX * rightTangentY) - (bottomTangentY * rightTangentX);
            // cross product right and top
            float rightNormalX = (rightTangentY * topTangentZ) - (rightTangentZ * topTangentY);
            float rightNormalY = (rightTangentZ * topTangentX) - (rightTangentX * topTangentZ);
            float rightNormalZ = (rightTangentX * topTangentY) - (rightTangentY * topTangentX);
        }
    }

    // Assign colors
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
