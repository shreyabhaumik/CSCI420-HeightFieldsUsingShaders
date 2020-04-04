#ifndef _OPENGLHEADER_H_
#define _OPENGLHEADER_H_

// headers of shader-based OpenGL

#if defined(WIN32) || defined(_WIN32) || defined(linux) || defined(__linux__)
  #include <GL/glew.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl3.h>
  #include <OpenGL/gl3ext.h>
#endif

#endif

