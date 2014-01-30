/////////////////////////////////////////////
//
// Geometry Clip-Maps Tutorial
//
// (C) by Sven Forstmann in 2014
//
// License : MIT
// http://opensource.org/licenses/MIT
/////////////////////////////////////////////
// Mathlib included from 
// http://sourceforge.net/projects/nebuladevice/
/////////////////////////////////////////////
#include <iostream> 
#include <vector> 
#include <string> 
#include <stdio.h>
#include <glew.h>
#include <wglew.h>
#include <windows.h>
#include <mmsystem.h>
#include <GL/glut.h>
using namespace std;
#include "glsl.h"
#pragma comment(lib,"winmm.lib")
///////////////////////////////////////////
#include "Bmp.h"
#include "ogl.h"
///////////////////////////////////////////
int grid=64;				// patch resolution
int levels=6;				// LOD levels
int width=4096,height=4096; // heightmap dimensions
///////////////////////////////////////////
void DrawScene()
{
	POINT cursor;
	GetCursorPos(&cursor); // mouse pointer position

	bool	wireframe= GetAsyncKeyState(VK_SPACE);	// render wireframe
	bool	topdown	 = GetAsyncKeyState(VK_RETURN);	// view top-down
	float	viewangle= float(cursor.x)/5.0;
	vec3f	viewpos ( timeGetTime()&65535 , -(float(cursor.y)/1000.0)* 0.1-0.01 , 0 );

	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	static int bmp_tex=0;

	static bool init=true;
	static Shader shader("Shader");

	static int vbo=0;
	static std::vector<float> vert;

	if(init)
	{
		/*+++++++++++++++++++++++++++++++++++++*/
		// make simple sine heightmap
		std::vector<float> data; 
		data.resize(width*height);
		loopj(0,height)
		loopi(0,width)
		{
			float a= float(i)/float(width);
			float b= float(j)/float(height);
			float h = (sin(4*M_PI*a)+sin(4*M_PI*b)+sin(16*M_PI*a)*sin(16*M_PI*b))*0.125+0.5;
			data[i+j*width]=h;
		}		
		bmp_tex = ogl_tex_new(width,height,GL_LINEAR,GL_REPEAT,GL_LUMINANCE16F_ARB,GL_LUMINANCE,(unsigned char*)&data[0], GL_FLOAT);
		/*+++++++++++++++++++++++++++++++++++++*/
		// driver info
		std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;			//std::cout << "GL_EXTENSIONS: " << glGetString(GL_EXTENSIONS) << std::endl;
		std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "GLU_VERSION: " << gluGetString(GLU_VERSION) << std::endl;			//std::cout << "GLU_EXTENSIONS: " << gluGetString(GLU_EXTENSIONS) << std::endl;
		std::cout << "GLUT_API_VERSION: " << GLUT_API_VERSION << std::endl;
		/*+++++++++++++++++++++++++++++++++++++*/
		// load shaders
		shader.attach(GL_VERTEX_SHADER,"../shader/vs.txt");
		shader.attach(GL_FRAGMENT_SHADER,"../shader/frag.txt");
		shader.link();
		/*+++++++++++++++++++++++++++++++++++++*/
		// make vbo quad patch
		loopj(0,grid+1)
		loopi(0,grid+2)
		{
			loopk(0, ((i==0) ? 2 : 1) )
			{
				vert.push_back(float(i)/grid);
				vert.push_back(float(j)/grid);
				vert.push_back(0);
			}			
			++j;
			loopk(0, ((i==grid+1) ? 2 : 1) )
			{
				vert.push_back(float(i)/grid);
				vert.push_back(float(j)/grid);
				vert.push_back(0);
			}
			--j;
		}
		/*+++++++++++++++++++++++++++++++++++++*/
		glGenBuffers(1, (GLuint *)(&vbo));
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vert.size(),&vert[0], GL_DYNAMIC_DRAW_ARB );
		/*+++++++++++++++++++++++++++++++++++++*/
		init=false;
		/*+++++++++++++++++++++++++++++++++++++*/
	}
	glMatrixMode( GL_PROJECTION);
	glLoadIdentity();

	if (topdown)
	{
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		glRotatef(180,1,0,0);
		wireframe^=1;
	}
	else		 
	{
		int vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		gluPerspective(90.0,float(vp[2])/float(vp[3]) , 0.0005, 10.0);
		glTranslatef(0,viewpos.y,0);	// set height
		glRotatef(130,1,0,0);		
		glRotatef(viewangle,0,0,1);		// set rotation
	}

	matrix44 mat;
	glGetFloatv(GL_PROJECTION_MATRIX, &mat.m[0][0]);		CHECK_GL_ERROR();
	
	// Enable VBO
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo);				CHECK_GL_ERROR();
	glEnableClientState(GL_VERTEX_ARRAY);					CHECK_GL_ERROR();
	glVertexPointer  ( 3, GL_FLOAT,0, (char *) 0);			CHECK_GL_ERROR();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, bmp_tex);

	// Triangle Mesh
	shader.begin();
	shader.setUniform1i("texTerrain",0);
	shader.setUniformMatrix4fv("projectionMatrix", 1, 0, &mat.m[0][0]);	

	float sxy=2; // scale x/y
	shader.setUniform4f("map_position", 
		-viewpos.x/float(2*512*grid),
		-viewpos.z/float(2*512*grid),0,0);

	loopi(0,levels)
	{
		float ox=((int(viewpos.x)<<i)&511)/float(512*grid);
		float oy=((int(viewpos.z)<<i)&511)/float(512*grid);

		loopk(-2,2) loopj(-2,2) // each level has 4x4 patches
		{
			if(i!=levels-1) if(k==-1||k==0) if(j==-1||j==0) continue;

			vec3f scale	(sxy*0.25,sxy*0.25,1);
			vec3f offset(ox+float(j),oy+float(k),0);

			if(k>=0) offset.y-=1.0/float(grid); // adjust offset for proper overlapping
			if(j>=0) offset.x-=1.0/float(grid); // adjust offset for proper overlapping

			//cull
			int xp=0,xm=0,yp=0,ym=0,zp=0;
			looplmn(0,0,0,2,2,2)
			{
				vec3f v = scale*(offset+vec3f(l,m,float(-n)*0.05)); // bbox vector
				vec4f cs = mat * vec4f(v.x,v.y,v.z,1); // clipspace
				if(cs.z< cs.w) zp++;				
				if(cs.x<-cs.w) xm++;	if(cs.x>cs.w) xp++;
				if(cs.y<-cs.w) ym++;	if(cs.y>cs.w) yp++;
			}
			if(zp==0 || xm==8 || xp==8 || ym==8 || yp==8)continue; // skip if invisible
			
			//render
			shader.setUniform4f("offset", offset.x,offset.y,0,0);
			shader.setUniform4f("scale" , scale.x,scale.y,1,1);	
			if(wireframe)	glDrawArrays( GL_LINES, 0, vert.size());
			else			glDrawArrays( GL_TRIANGLE_STRIP, 0, vert.size());
		}
		sxy*=0.5;
	}	
	shader.end();

	// Disable VBO
	glDisableClientState(GL_VERTEX_ARRAY);									CHECK_GL_ERROR();
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);								CHECK_GL_ERROR();
	glutSwapBuffers();
}
///////////////////////////////////////////
int main(int argc, char **argv) 
{ 
  glutInit(&argc, argv);  
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);  
  glutInitWindowSize(800, 600);  
  glutInitWindowPosition(0, 0);  
  glutCreateWindow("Geometry Clipmaps Example (c) Sven Forstmann 2014");
  glutDisplayFunc(DrawScene);
  glutIdleFunc(DrawScene);
  glewInit();
  wglSwapIntervalEXT(0);
  glutMainLoop();  
  return 0;
}
///////////////////////////////////////////
