#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>


#define GL_ERROR() checkForOpenGLError(__FILE__, __LINE__)
using namespace std;
using glm::mat4;
using glm::vec3;
static GLuint g_vao;
static GLuint g_programHandle;
static GLuint g_winWidth = 400;
static GLuint g_winHeight = 400;
static GLint g_angle = 0;
static GLuint g_frameBuffer;
// transfer function
static GLuint g_tffTexObj;
static GLuint g_bfTexObj;
static GLuint g_texWidth;
static GLuint g_texHeight;
static GLuint g_volTexObj;
static GLuint g_rcVertHandle;
static GLuint g_rcFragHandle;
static GLuint g_bfVertHandle;
static GLuint g_bfFragHandle;
static float g_stepSize = 0.001f;


static
int checkForOpenGLError(const char* file, int line)
{
    // return 1 if an OpenGL error occured, 0 otherwise.
    GLenum glErr;
    int retCode = 0;

    glErr = glGetError();
    while(glErr != GL_NO_ERROR)
    {
        cout << "glError in file " << file << "@line " << line << ": "
             << gluErrorString(glErr) << endl;
        retCode = 1;
        exit(EXIT_FAILURE);
    }
    return retCode;
}


// Init the  vertex buffer object for  a surface (here a  surface of a
// cube)  colored by  3d-position  of the  surface  point.  Returns  a
// VAO. FIXME: leaks 2 VBOs.
static
GLuint init_vertex_objects ()
{
    // Coordinates  of the  eight cube  corners which  are  also their
    // colors by the very construction:
    GLfloat vertices[24] = {
        0.0, 0.0, 0.0,          // 0
        0.0, 0.0, 1.0,          // 1
        0.0, 1.0, 0.0,          // 2
        0.0, 1.0, 1.0,          // 3
        1.0, 0.0, 0.0,          // 4
        1.0, 0.0, 1.0,          // 5
        1.0, 1.0, 0.0,          // 6
        1.0, 1.0, 1.0           // 7
    };
    // Draw the  six faces of  the bounding box by  drawing triangles.
    // Draw it counter-clockwise.
    //
    // front: 1 5 7 3
    // back:  0 2 6 4
    // left:  0 1 3 2
    // right: 7 5 4 6
    // up:    2 3 7 6
    // down:  1 0 4 5
    GLuint indices[36] = {
        1, 5, 7,
        7, 3, 1,
        0, 2, 6,
        6, 4, 0,
        0, 1, 3,
        3, 2, 0,
        7, 5, 4,
        4, 6, 7,
        2, 3, 7,
        7, 6, 2,
        1, 0, 4,
        4, 5, 1
    };

    // FIXME:   These  two   vertex  buffer   objects   (VBOs)  become
    // inaccessible on return:
    GLuint gbo[2];
    glGenBuffers (2, gbo);

    GLuint vertexdat = gbo[0];
    GLuint veridxdat = gbo[1];

    glBindBuffer (GL_ARRAY_BUFFER, vertexdat);
    glBufferData (GL_ARRAY_BUFFER, 24 * sizeof (GLfloat), vertices, GL_STATIC_DRAW);
    // used in glDrawElement()
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, veridxdat);
    glBufferData (GL_ELEMENT_ARRAY_BUFFER, 36 * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // A vertex array  object (VAO) is like a  closure binding several
    // buffer  objects.   Here  ---  the vertex  locations  and  their
    // colors:
    GLuint vao;
    glGenVertexArrays (1, &vao);
    glBindVertexArray (vao);
    glEnableVertexAttribArray (0); // for vertex location
    glEnableVertexAttribArray (1); // for vertex color

    // The vertex  location is the same  as the vertex  color. Use the
    // same  buffer for  both  attributes (VerPos  and  VerClr in  the
    // backface vertex shader):
    glBindBuffer (GL_ARRAY_BUFFER, vertexdat);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *) NULL);
    glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 0, (GLfloat *) NULL);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, veridxdat);
    // glBindVertexArray(0);

    return vao;
}


static
void drawBox(GLenum glFaces)
{
    glEnable(GL_CULL_FACE);
    glCullFace(glFaces);
    glBindVertexArray(g_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, (GLuint *)NULL);
    glDisable(GL_CULL_FACE);
}


// Check the compilation result. Print errors and eventual warnings to
// stderr. See also checkShaderLinkStatus().
static
GLboolean compileCheck (GLuint shader)
{
    GLint err;
    glGetShaderiv (shader, GL_COMPILE_STATUS, &err);

    // Print errors *and* warnings to stderr unconditionally, not only
    // if (GL_FALSE == err)
    {
        // Number  of characters  in  the information  log for  shader
        // *including* the null termination character:
        GLint logLen;
        glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &logLen);

        if (logLen > 1)
        {
            char* log = (char *) malloc (logLen);
            GLsizei written;
            glGetShaderInfoLog (shader, logLen, &written, log);
            cerr << "Shader log: " << log << endl;
            free (log);
        }
    }
    return err;
}


// init shader object
static
GLuint initShaderObj(const GLchar* srcfile, GLenum shaderType)
{
    ifstream inFile(srcfile, ifstream::in);
    // use assert?
    if (!inFile)
    {
        cerr << "Error openning file: " << srcfile << endl;
        exit(EXIT_FAILURE);
    }

    const int MAX_CNT = 10000;
    GLchar *shaderCode = (GLchar *) calloc(MAX_CNT, sizeof(GLchar));
    inFile.read(shaderCode, MAX_CNT);
    if (inFile.eof())
    {
        size_t bytecnt = inFile.gcount();
        *(shaderCode + bytecnt) = '\0';
    }
    else if(inFile.fail())
        cout << srcfile << "read failed " << endl;
    else
        cout << srcfile << "is too large" << endl;

    // create the shader Object
    GLuint shader = glCreateShader(shaderType);
    if (0 == shader)
        cerr << "Error creating vertex shader." << endl;

    // cout << shaderCode << endl;
    // cout << endl;
    const GLchar* codeArray[] = {shaderCode};
    glShaderSource(shader, 1, codeArray, NULL);
    free(shaderCode);

    // compile the shader
    glCompileShader(shader);
    if (GL_FALSE == compileCheck(shader))
        cerr << "shader compilation failed" << endl;

    return shader;
}


// Check the result of linking.  Print errors and eventual warnings to
// stderr. See also compileCheck().
static
GLint checkShaderLinkStatus (GLuint pgmHandle)
{
    GLint status;
    glGetProgramiv (pgmHandle, GL_LINK_STATUS, &status);

    // Print errors *and* warnings to stderr unconditionally, not only
    // if (GL_FALSE == status)
    {
        // Number of characters in the information log for *including*
        // the null termination character:
        GLint logLen;
        glGetProgramiv (pgmHandle, GL_INFO_LOG_LENGTH, &logLen);

        if (logLen > 1)
        {
            GLchar * log = (GLchar *) malloc(logLen);
            GLsizei written;
            glGetProgramInfoLog (pgmHandle, logLen, &written, log);
            cerr << "Program log: " << log << endl;
        }
    }
    return status;
}


// link shader program
static
GLuint createShaderPgm()
{
    // Create the shader program
    GLuint programHandle = glCreateProgram();
    if (0 == programHandle)
    {
        cerr << "Error create shader program" << endl;
        exit(EXIT_FAILURE);
    }
    return programHandle;
}


// init the 1 dimentional texture for transfer function
static
GLuint initTFF1DTex(const char* filename)
{
    // read in the user defined data of transfer function
    ifstream inFile(filename, ifstream::in);
    if (!inFile)
    {
        cerr << "Error openning file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    const int MAX_CNT = 10000;
    GLubyte *tff = (GLubyte *) calloc(MAX_CNT, sizeof(GLubyte));
    inFile.read(reinterpret_cast<char *>(tff), MAX_CNT);
    if (inFile.eof())
    {
        size_t bytecnt = inFile.gcount();
        *(tff + bytecnt) = '\0';
        cout << "bytecnt " << bytecnt << endl;
    }
    else if(inFile.fail())
        cout << filename << "read failed " << endl;
    else
        cout << filename << "is too large" << endl;

    GLuint tff1DTex;
    glGenTextures(1, &tff1DTex);
    glBindTexture(GL_TEXTURE_1D, tff1DTex);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, tff);
    free(tff);
    return tff1DTex;
}


// init the 2D texture for render backface 'bf' stands for backface
static
GLuint initFace2DTex(GLuint bfTexWidth, GLuint bfTexHeight)
{
    GLuint backFace2DTex;
    glGenTextures(1, &backFace2DTex);
    glBindTexture(GL_TEXTURE_2D, backFace2DTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, bfTexWidth, bfTexHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    return backFace2DTex;
}


// Init 3D texture to store the volume data used for ray casting.
static
GLuint initVol3DTex (const char* filename, GLuint w, GLuint h, GLuint d)
{
    const size_t size = w * h * d;
    GLubyte *data = new GLubyte[size]; // 8bit

    {
        FILE *fp = fopen (filename, "rb");

        if (!fp)
        {
            cerr << "Error: opening file failed" << endl;
            exit (EXIT_FAILURE);
        }

        if (fread (data, sizeof (char), size, fp) != size)
        {
            cerr << "Error: reading file failed" << endl;
            exit (EXIT_FAILURE);
        }

        fclose (fp);
    }

    GLuint volTexObj;           // result
    glGenTextures (1, &volTexObj);

    // Bind 3D texture target:
    glBindTexture (GL_TEXTURE_3D, volTexObj);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri (GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    // Pixel transfer happens here from client to OpenGL server:
    glPixelStorei (GL_UNPACK_ALIGNMENT,1);
    glTexImage3D (GL_TEXTURE_3D, 0, GL_INTENSITY, w, h, d, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,data);

    delete[] data;

    return volTexObj;
}


static
void checkFramebufferStatus()
{
    GLenum complete = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (complete != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "framebuffer is not complete" << endl;
        exit(EXIT_FAILURE);
    }
}


// init the framebuffer, the only framebuffer used in this program
static
void initFrameBuffer(GLuint texObj, GLuint texWidth, GLuint texHeight)
{
    // create a depth buffer for our framebuffer
    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, texWidth, texHeight);

    // attach the texture and the depth buffer to the framebuffer
    glGenFramebuffers(1, &g_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texObj, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    checkFramebufferStatus();
    glEnable(GL_DEPTH_TEST);
}


static
void rcSetUinforms()
{
    // setting uniforms such as
    // ScreenSize
    // StepSize
    // TransferFunc
    // ExitPoints i.e. the backface, the backface hold the ExitPoints of ray casting
    // VolumeTex the texture that hold the volume data i.e. head256.raw
    GLint screenSizeLoc = glGetUniformLocation(g_programHandle, "ScreenSize");
    if (screenSizeLoc >= 0)
        glUniform2f(screenSizeLoc, (float)g_winWidth, (float)g_winHeight);
    else
        cout << "ScreenSize is not bound to the uniform" << endl;

    GLint stepSizeLoc = glGetUniformLocation(g_programHandle, "StepSize");
    GL_ERROR();
    if (stepSizeLoc >= 0)
        glUniform1f(stepSizeLoc, g_stepSize);
    else
        cout << "StepSize is not bound to the uniform" << endl;

    GL_ERROR();
    GLint transferFuncLoc = glGetUniformLocation(g_programHandle, "TransferFunc");
    if (transferFuncLoc >= 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, g_tffTexObj);
        glUniform1i(transferFuncLoc, 0);
    }
    else
        cout << "TransferFunc is not bound to the uniform" << endl;

    GL_ERROR();
    GLint backFaceLoc = glGetUniformLocation(g_programHandle, "ExitPoints");
    if (backFaceLoc >= 0)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_bfTexObj);
        glUniform1i(backFaceLoc, 1);
    }
    else
        cout << "ExitPoints is not bound to the uniform" << endl;

    GL_ERROR();
    GLint volumeLoc = glGetUniformLocation(g_programHandle, "VolumeTex");
    if (volumeLoc >= 0)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, g_volTexObj);
        glUniform1i(volumeLoc, 2);
    }
    else
        cout << "VolumeTex is not bound to the uniform" << endl;
}


// init the shader object and shader program
static
void initShader()
{
// vertex shader object for first pass
    g_bfVertHandle = initShaderObj("shader/backface.vert", GL_VERTEX_SHADER);
// fragment shader object for first pass
    g_bfFragHandle = initShaderObj("shader/backface.frag", GL_FRAGMENT_SHADER);
// vertex shader object for second pass
    g_rcVertHandle = initShaderObj("shader/raycasting.vert", GL_VERTEX_SHADER);
// fragment shader object for second pass
    g_rcFragHandle = initShaderObj("shader/raycasting.frag", GL_FRAGMENT_SHADER);
// create the shader program , use it in an appropriate time
    g_programHandle = createShaderPgm();
    // Obtained  indices distributed  by  shader compiler  (optional).
    // [original: 获得由着色器编译器分配的索引 (可选)]
}


// link the shader objects using the shader program
static
void linkShader(GLuint shaderPgm, GLuint newVertHandle, GLuint newFragHandle)
{
    const GLsizei maxCount = 2;
    GLsizei count;
    GLuint shaders[maxCount];
    glGetAttachedShaders(shaderPgm, maxCount, &count, shaders);
    // cout << "get VertHandle: " << shaders[0] << endl;
    // cout << "get FragHandle: " << shaders[1] << endl;
    GL_ERROR();
    for (int i = 0; i < count; i++)
        glDetachShader(shaderPgm, shaders[i]);

    // Bind index 0 to the shader input variable "VerPos"
    glBindAttribLocation(shaderPgm, 0, "VerPos");
    // Bind index 1 to the shader input variable "VerClr"
    glBindAttribLocation(shaderPgm, 1, "VerClr");
    GL_ERROR();
    glAttachShader(shaderPgm,newVertHandle);
    glAttachShader(shaderPgm,newFragHandle);
    GL_ERROR();
    glLinkProgram(shaderPgm);
    if (GL_FALSE == checkShaderLinkStatus(shaderPgm))
    {
        cerr << "Failed to relink shader program!" << endl;
        exit(EXIT_FAILURE);
    }
    GL_ERROR();
}


// both of the two pass use the "render() function"
// the first pass render the backface of the boundbox
// the second pass render the frontface of the boundbox
// together with the frontface, use the backface as a 2D texture in the second pass
// to calculate the entry point and the exit point of the ray in and out the box.
static
void render(GLenum cullFace)
{
    GL_ERROR();
    glClearColor(0.2f,0.2f,0.2f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //  transform the box
    glm::mat4 projection = glm::perspective(60.0f, (GLfloat)g_winWidth/g_winHeight, 0.1f, 400.f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f),
                                 glm::vec3(0.0f, 0.0f, 0.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = mat4(1.0f);
    model *= glm::rotate((float)g_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    // to make the "head256.raw" i.e. the volume data stand up.
    model *= glm::rotate(90.0f, vec3(1.0f, 0.0f, 0.0f));
    model *= glm::translate(glm::vec3(-0.5f, -0.5f, -0.5f));
    // notice the multiplication order: reverse order of transform
    glm::mat4 mvp = projection * view * model;
    GLuint mvpIdx = glGetUniformLocation(g_programHandle, "MVP");
    if (mvpIdx >= 0)
        glUniformMatrix4fv(mvpIdx, 1, GL_FALSE, &mvp[0][0]);
    else
        cerr << "can't get the MVP" << endl;

    GL_ERROR();
    drawBox(cullFace);
    GL_ERROR();
    // glutWireTeapot(0.5);
}


// the color of the vertex in the back face is also the location
// of the vertex
// save the back face to the user defined framebuffer bound
// with a 2D texture named `g_bfTexObj`
// draw the front face of the box
// in the rendering process, i.e. the ray marching process
// loading the volume `g_volTexObj` as well as the `g_bfTexObj`
// after vertex shader processing we got the color as well as the location of
// the vertex (in the object coordinates, before transformation).
// and the vertex assemblied into primitives before entering
// fragment shader processing stage.
// in fragment shader processing stage. we got `g_bfTexObj`
// (correspond to 'VolumeTex' in glsl)and `g_volTexObj`(correspond to 'ExitPoints')
// as well as the location of primitives.
// the most important is that we got the GLSL to exec the logic. Here we go!
// draw the back face of the box
static
void display()
{
    glEnable(GL_DEPTH_TEST);
    // test the gl_error
    GL_ERROR();
    // render to texture
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_frameBuffer);
    glViewport(0, 0, g_winWidth, g_winHeight);
    linkShader(g_programHandle, g_bfVertHandle, g_bfFragHandle);
    glUseProgram(g_programHandle);
    // cull front face
    render(GL_FRONT);
    glUseProgram(0);
    GL_ERROR();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_winWidth, g_winHeight);
    linkShader(g_programHandle, g_rcVertHandle, g_rcFragHandle);
    GL_ERROR();
    glUseProgram(g_programHandle);
    rcSetUinforms();
    GL_ERROR();
    // glUseProgram(g_programHandle);
    // cull back face
    render(GL_BACK);
    // need or need not to resume the state of only one active texture unit?
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glDisable(GL_TEXTURE_2D);
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_3D, 0);
    // glDisable(GL_TEXTURE_3D);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_1D, 0);
    // glDisable(GL_TEXTURE_1D);
    // glActiveTexture(GL_TEXTURE0);
    glUseProgram(0);
    GL_ERROR();

    // // for test the first pass
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, g_frameBuffer);
    // checkFramebufferStatus();
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    // glViewport(0, 0, g_winWidth, g_winHeight);
    // glClearColor(0.0, 0.0, 1.0, 1.0);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // GL_ERROR();
    // glBlitFramebuffer(0, 0, g_winWidth, g_winHeight,0, 0,
    //                g_winWidth, g_winHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // GL_ERROR();
    glutSwapBuffers();
}


static
void rotateDisplay()
{
    g_angle = (g_angle + 1) % 360;
    glutPostRedisplay();
}


static
void reshape(int w, int h)
{
    g_winWidth = w;
    g_winHeight = h;
    g_texWidth = w;
    g_texHeight = h;
}


static
void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'q':
    case '\x1B':
        exit(EXIT_SUCCESS);
        break;
    }
}


static
void init()
{
    g_texWidth = g_winWidth;
    g_texHeight = g_winHeight;
    g_vao = init_vertex_objects ();
    initShader();
    g_tffTexObj = initTFF1DTex("tff.dat");
    g_bfTexObj = initFace2DTex(g_texWidth, g_texHeight);
    g_volTexObj = initVol3DTex ("head256.raw", 256, 256, 225);
    GL_ERROR();
    initFrameBuffer(g_bfTexObj, g_texWidth, g_texHeight);
    GL_ERROR();
}


int main(int argc, char** argv)
{

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize (g_winWidth, g_winHeight);
    glutCreateWindow("GLUT Test");
    GLenum err = glewInit();
    if (GLEW_OK != err)
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));

    glutKeyboardFunc(&keyboard);
    glutDisplayFunc(&display);
    glutReshapeFunc(&reshape);
    glutIdleFunc(&rotateDisplay);
    init();
    glutMainLoop();
    /* NOTREACHED */
    return EXIT_SUCCESS;
}

