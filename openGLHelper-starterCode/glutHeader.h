#ifndef _GLUT_HEADER_H_
#define _GLUT_HEADER_H_

// headers of the GLUT library

#if defined(WIN32) || defined(_WIN32) || defined(linux) || defined(__linux__)
  #include <GL/glut.h>
  //#include <GL/freeglut_ext.h>
#elif defined(__APPLE__)
  #define __gl_h_
  #include <GLUT/glut.h>
#endif

#endif

