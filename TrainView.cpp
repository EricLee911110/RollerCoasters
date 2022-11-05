/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "GL/glu.h"

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"


#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized glad
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
	}
	else
		throw std::runtime_error("Could not initialize GLAD!");

	// Set up the view port
	glViewport(0,0,w(),h());

	// clear the window, be sure to clear the Z-Buffer too
	glClearColor(0,0,.3f,0);		// background should be blue

	// we need to clear out the stencil buffer since we'll use
	// it for shadows
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH);

	// Blayne prefers GL_DIFFUSE
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// prepare for projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	setProjection();		// put the code to set up matrices here

	//######################################################################
	// TODO: 
	// you might want to set the lighting up differently. if you do, 
	// we need to set up the lights AFTER setting up the projection
	//######################################################################
	// enable the lighting
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// top view only needs one light
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	} else {
		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHT2);
	}

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	
	GLfloat lightPosition1[]	= {0,1,0,0}; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition2[]	= {0, 0.01, 0, 1.0};
	GLfloat lightPosition3[]	= {0, 0.01, 0, 1.0};
	GLfloat yellowLight[]		= {0.5f, 0.5f, .1f, 1.0};
	GLfloat whiteLight[]			= {1.0f, 1.0f, 1.0f, 1.0};
	GLfloat blueLight[]			= {.1f,.1f,.3f,1.0};
	GLfloat grayLight[]			= {.3f, .3f, .3f, 1.0};
	GLfloat redLight[]			= { 1.0f, 0.0f, 0.0f, 1.0f };
	GLfloat blackLight[]		= { 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat spot_dir[]			= { 1.0, 0.0, 0.0 };
	

	
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, blackLight);

	
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, redLight);

	
	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, yellowLight);
	glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spot_dir);
	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0f);


	
	
	/*
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);
	*/


	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(200,10);


	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	glEnable(GL_LIGHTING);
	setupObjects();

	drawStuff();

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	else if (tw->trainCam->value()) {

		glClear(GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45, aspect, 0.01f, 1000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		float local_current_length = current_length;
		int local_start_point = 0;



		for (int i = 0; i < copy_list_sum_track_length.size(); i++) {
			if ((int)(local_current_length / copy_list_sum_track_length[i]) >= 1) {
				//std::cout << "I checked" << std::endl;
				local_start_point = (i + 1) % (copy_list_sum_track_length.size());
			}

		}

		if (tw->splineBrowser->value() == 1) {
			if (tw->arcLength->value()) {

				Pnt3f train_cp_pos_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].pos;

				Pnt3f train_cp_orient_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].orient;

				float train_percentage;
				if (local_start_point == 0) {
					train_percentage = local_current_length / copy_list_sum_track_length[0];
				}
				else {
					train_percentage = (local_current_length - copy_list_sum_track_length[local_start_point - 1]) / (copy_list_sum_track_length[local_start_point] - copy_list_sum_track_length[local_start_point - 1]);
				}

				Pnt3f qt0 = train_cp_pos_p2 * train_percentage + train_cp_pos_p1 * (1 - train_percentage);
				Pnt3f qt1 = train_cp_pos_p2 * (train_percentage + 0.0001) + train_cp_pos_p1 * (1 - train_percentage - 0.0001);

				Pnt3f forward = qt1 - qt0;
				forward.normalize();
				Pnt3f orient = train_cp_orient_p2 * (train_percentage)+train_cp_orient_p1 * (1 - train_percentage);
				orient.normalize();

				Pnt3f right = forward * orient;
				right.normalize();
				Pnt3f up = right * forward;
				up.normalize();

				Pnt3f this_pos = qt0 + up * 5.0f;
				Pnt3f next_pos = qt1 + up * 5.0f;
				gluLookAt(this_pos.x, this_pos.y, this_pos.z, next_pos.x, next_pos.y, next_pos.z, up.x, up.y, up.z);
			}
			else {
				//	without arc-length
				
				Pnt3f train_cp_pos_p1 = m_pTrack->points[(int)t_time].pos;
				Pnt3f train_cp_pos_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].pos;

				Pnt3f train_cp_orient_p1 = m_pTrack->points[(int)t_time].orient;
				Pnt3f train_cp_orient_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].orient;

				float train_percentage = t_time - (int)t_time;
				Pnt3f qt0 = train_cp_pos_p2 * train_percentage + train_cp_pos_p1 * (1 - train_percentage);
				Pnt3f qt1 = train_cp_pos_p2 * (train_percentage + 0.0001) + train_cp_pos_p1 * (1 - train_percentage - 0.0001);

				Pnt3f forward = qt1 - qt0;
				forward.normalize();
				Pnt3f orient = train_cp_orient_p2 * (train_percentage)+train_cp_orient_p1 * (1 - train_percentage);
				orient.normalize();

				Pnt3f right = forward * orient;
				right.normalize();
				Pnt3f up = right * forward;
				up.normalize();
				
				Pnt3f this_pos = qt0 + up * 5.0f;
				Pnt3f next_pos = qt1 + up * 5.0f;
				gluLookAt(this_pos.x, this_pos.y, this_pos.z, next_pos.x, next_pos.y, next_pos.z, up.x, up.y, up.z);
			}
		}

		if (tw->splineBrowser->value() >= 2) {

			if (tw->arcLength->value()) {

				Pnt3f train_cp_pos_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p3 = m_pTrack->points[(local_start_point + 2) % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p4 = m_pTrack->points[(local_start_point + 3) % m_pTrack->points.size()].pos;


				Pnt3f train_cp_orient_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p3 = m_pTrack->points[(local_start_point + 2) % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p4 = m_pTrack->points[(local_start_point + 3) % m_pTrack->points.size()].orient;



				float train_percentage;
				if (local_start_point == 0) {
					train_percentage = local_current_length / copy_list_sum_track_length[0];
				}
				else {
					if (copy_list_sum_track_length.size() > local_start_point)
						train_percentage = (local_current_length - copy_list_sum_track_length[local_start_point - 1]) / (copy_list_sum_track_length[local_start_point] - copy_list_sum_track_length[local_start_point - 1]);
					else {
						std::cout << "copy: " << copy_list_sum_track_length.size() << " local_start_point " << local_start_point << std::endl;
						train_percentage = 0;
					}
				}

				Pnt3f qt0 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, train_percentage, tw->splineBrowser->value());
				Pnt3f qt1 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, train_percentage + 0.0001, tw->splineBrowser->value());

				Pnt3f orient = GMT(train_cp_orient_p1, train_cp_orient_p2, train_cp_orient_p3, train_cp_orient_p4, train_percentage, tw->splineBrowser->value());

				Pnt3f forward = qt1 - qt0;

				forward.normalize();
				orient.normalize();

				Pnt3f right = forward * orient;
				right.normalize();
				Pnt3f up = right * forward;
				up.normalize();

				Pnt3f this_pos = qt0 + up * 5.0f;
				Pnt3f next_pos = qt1 + up * 5.0f;
				gluLookAt(this_pos.x, this_pos.y, this_pos.z, next_pos.x, next_pos.y, next_pos.z, up.x, up.y, up.z);
			}
			else {
				Pnt3f train_cp_pos_p1 = m_pTrack->points[(int)t_time].pos;
				Pnt3f train_cp_pos_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p3 = m_pTrack->points[((int)t_time + 2) % m_pTrack->points.size()].pos;
				Pnt3f train_cp_pos_p4 = m_pTrack->points[((int)t_time + 3) % m_pTrack->points.size()].pos;

				Pnt3f train_cp_orient_p1 = m_pTrack->points[(int)t_time].orient;
				Pnt3f train_cp_orient_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p3 = m_pTrack->points[((int)t_time + 2) % m_pTrack->points.size()].orient;
				Pnt3f train_cp_orient_p4 = m_pTrack->points[((int)t_time + 3) % m_pTrack->points.size()].orient;

				Pnt3f qt0 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, t_time - (int)t_time, tw->splineBrowser->value());
				Pnt3f qt1 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, t_time - (int)t_time + 0.0001, tw->splineBrowser->value());

				Pnt3f orient = GMT(train_cp_orient_p1, train_cp_orient_p2, train_cp_orient_p3, train_cp_orient_p4, t_time - (int)t_time, tw->splineBrowser->value());

				Pnt3f forward = qt1 - qt0;

				forward.normalize();
				orient.normalize();

				Pnt3f right = forward * orient;
				right.normalize();
				Pnt3f up = right * forward;
				up.normalize();

				Pnt3f this_pos = qt0 + up * 5.0f;
				Pnt3f next_pos = qt1 + up * 5.0f;
				gluLookAt(this_pos.x, this_pos.y, this_pos.z, next_pos.x, next_pos.y, next_pos.z, up.x, up.y, up.z);
			}
		}

	}
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================

void TrainView::drawStuff(bool doingShadows)
{
	std::vector<float> list_sum_track_length;

	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if (((int)i) != selectedCube)
					glColor3ub(240, 60, 60);
				else {
					glColor3ub(240, 240, 30);
				}
			}
			m_pTrack->points[i].draw();
		}
		
	}
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################
	
	
	if (tw->splineBrowser->value() == 1) {	// Linear
		float total_length = 0;
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {

			// pos
			Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
			Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;
			// orient
			Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
			Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;
			float percent = 1.0f / DIVIDE_LINE;
			float t = 0;
			Pnt3f qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;

			float two_cp_length = 0;

			for (size_t j = 0; j < DIVIDE_LINE; j++) {
				Pnt3f qt0 = qt;
				t += percent;
				qt = (1 - t) * cp_pos_p1 + t * cp_pos_p2;
				Pnt3f qt1 = qt;
				
				//	get length
				two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));

				glLineWidth(3);
				glBegin(GL_LINES);
				if (!doingShadows)
					glColor3ub(32, 32, 64);
				glVertex3f(qt0.x, qt0.y, qt0.z);
				glVertex3f(qt1.x, qt1.y, qt1.z);
				glEnd();

				// cross


				Pnt3f orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t.normalize();
				// Pnt3f forward = 
				Pnt3f cross_t = (qt1 - qt0) * orient_t;

				cross_t.normalize();
				cross_t = cross_t * 2.5f;
				glBegin(GL_LINES);
				glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
				glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
				glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
				glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
				glEnd();
				glLineWidth(1);




			}

			total_length += two_cp_length;
			list_track_length.push_back(two_cp_length);
			list_sum_track_length.push_back(total_length);
		}
		int length_list_track_length = list_track_length.size();
		tw->tv_length_list_track_length = length_list_track_length;
		tw->tv_list_track_length = list_track_length;
		
		copy_list_sum_track_length = list_sum_track_length;
		/*
		for (int my_i = 0; my_i < length_list_track_length; my_i++) {
			std::cout << "track " << my_i << "'s length is: " << list_sum_track_length[my_i] << std::endl;
		}
		*/

		for (int my_i = 0; my_i < length_list_track_length; my_i++) {
			list_track_length.pop_back();
			list_sum_track_length.pop_back();
		}

		/*
		for (int i = 0; i < copy_list_sum_track_length.size(); i++) {
			std::cout << "track " << i << "'s length is: " << copy_list_sum_track_length[i] << std::endl;
		}
		*/

	}
	else if (tw->splineBrowser->value() >= 2) { //
		float total_length = 0;
		for (size_t i = 0; i < m_pTrack->points.size(); ++i) {
			// pos
			Pnt3f cp_pos_p1 = m_pTrack->points[i].pos;
			Pnt3f cp_pos_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].pos;
			Pnt3f cp_pos_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].pos;
			Pnt3f cp_pos_p4 = m_pTrack->points[(i + 3) % m_pTrack->points.size()].pos;
			// orient
			Pnt3f cp_orient_p1 = m_pTrack->points[i].orient;
			Pnt3f cp_orient_p2 = m_pTrack->points[(i + 1) % m_pTrack->points.size()].orient;
			Pnt3f cp_orient_p3 = m_pTrack->points[(i + 2) % m_pTrack->points.size()].orient;
			Pnt3f cp_orient_p4 = m_pTrack->points[(i + 3) % m_pTrack->points.size()].orient;

			float percent = 1.0f / DIVIDE_LINE;
			float t = 0;
			Pnt3f qt = GMT(cp_pos_p1, cp_pos_p2, cp_pos_p3, cp_pos_p4, t, tw->splineBrowser->value());

			float two_cp_length = 0;

			for (size_t j = 0; j < DIVIDE_LINE; j++) {
				Pnt3f qt0 = qt;
				t += percent;
				qt = GMT(cp_pos_p1, cp_pos_p2, cp_pos_p3, cp_pos_p4, t, tw->splineBrowser->value());
				Pnt3f qt1 = qt;

				two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));



				glLineWidth(3);
				glBegin(GL_LINES);
				if (!doingShadows)
					glColor3ub(32, 32, 64);
				glVertex3f(qt0.x, qt0.y, qt0.z);
				glVertex3f(qt1.x, qt1.y, qt1.z);
				glEnd();

			}
			
			total_length += two_cp_length;
			list_track_length.push_back(two_cp_length);
			list_sum_track_length.push_back(total_length);
			
		}
		
		int length_list_track_length = list_track_length.size();
		tw->tv_length_list_track_length = length_list_track_length;
		tw->tv_list_track_length = list_track_length;

		copy_list_sum_track_length = list_sum_track_length;

		for (int my_i = 0; my_i < length_list_track_length; my_i++) {
			list_track_length.pop_back();
			list_sum_track_length.pop_back();
		}
		


	}

	if (!tw->trainCam->value()) {
		drawTrain(doingShadows, 0);
	}
#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
	
}


Pnt3f TrainView::GMT(const Pnt3f& pt0, const Pnt3f& pt1, const Pnt3f& pt2, const Pnt3f& pt3, float t, int type) {
	glm::mat4x4 M;
	switch (type) {
	case 2: {
		M = {
			-1, 2, -1, 0,
			3, -5, 0, 2,
			-3, 4, 1, 0,
			1, -1, 0, 0
		};
		M /= 2.0f;
	}break;
	case 3: {
		M = {
			-1, 3, -3, 1,
			3, -6, 0, 4,
			-3, 3, 3, 1,
			1, 0, 0, 0
		};
		M /= 6.0f;
	}break;
	}

	M = glm::transpose(M);
	glm::mat4x4 G = {
		{pt0.x, pt0.y, pt0.z, 1.0f},
		{pt1.x, pt1.y, pt1.z, 1.0f},
		{pt2.x, pt2.y, pt2.z, 1.0f},
		{pt3.x, pt3.y, pt3.z, 1.0f}
	};

	glm::vec4 T = { t * t * t, t * t, t, 1.0f };
	glm::vec4 result = G * M * T;
	return Pnt3f(result[0], result[1], result[2]);

}

float points[][3] = {
	{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0},
	{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0}, {1.0, 1.0, 0.0},
	{1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}
};

int face[][4] = {
	{0, 1, 2, 3}, {7, 6, 5, 4}, {0,4,5,1}, {1,5,6,2}, {3,2,6,7}, {0,3,7,4}
};

float myColor[][3] = {
	{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0},
	{1.0, 1.0, 0.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}
};

void TrainView::drawCube(bool doingShadows) {
	int i;
	for (i = 0; i < 6; i++) {
		if (!doingShadows) {
			glColor3fv(myColor[i]);
		}
		glBegin(GL_POLYGON);
		glVertex3fv(points[face[i][0]]);
		glVertex3fv(points[face[i][1]]);
		glVertex3fv(points[face[i][2]]);
		glVertex3fv(points[face[i][3]]);
		glEnd();

	}
}

void TrainView::drawTrain(bool doingShadows, float backward_distance) {
	Pnt3f qt;
	Pnt3f forward;
	Pnt3f right;
	Pnt3f up;
	// set start_point for arc-length
	
	float local_current_length = current_length - backward_distance;
	int local_start_point = 0;
	
	if (local_current_length < 0) {
		local_current_length += copy_list_sum_track_length[copy_list_sum_track_length.size() - 1];
	}

	
	for (int i = 0; i < copy_list_sum_track_length.size(); i++) {
		if ((int)(local_current_length / copy_list_sum_track_length[i]) >= 1) {
			//std::cout << "I checked" << std::endl;
			local_start_point = (i + 1) % (copy_list_sum_track_length.size());
		}

	}

	//std::cout << "this start point" << local_start_point << std::endl;
	//std::cout << "local_current_length: " << local_current_length << " total_length: " << copy_list_sum_track_length[3] << "size: " << copy_list_sum_track_length.size() <<  std::endl;
	//std::cout << copy_list_sum_track_length[0] << " " << copy_list_sum_track_length[1] << " " << copy_list_sum_track_length[2] << " " << copy_list_sum_track_length[3] << std::endl;

	
	if (tw->splineBrowser->value() == 1) {

		if (tw->arcLength->value()) {

			Pnt3f train_cp_pos_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].pos;

			Pnt3f train_cp_orient_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].orient;

			float train_percentage;
			if (local_start_point == 0) {
				train_percentage = local_current_length / copy_list_sum_track_length[0];
			}
			else {
				train_percentage = (local_current_length - copy_list_sum_track_length[local_start_point - 1]) / (copy_list_sum_track_length[local_start_point] - copy_list_sum_track_length[local_start_point - 1]);
			}

			qt = train_cp_pos_p2 * train_percentage + train_cp_pos_p1 * (1 - train_percentage);
			Pnt3f qt1 = train_cp_pos_p2 * (train_percentage + 0.0001) + train_cp_pos_p1 * (1 - train_percentage - 0.0001);

			forward = qt1 - qt;
			forward.normalize();
			Pnt3f orient = train_cp_orient_p2 * (train_percentage)+train_cp_orient_p1 * (1 - train_percentage);
			orient.normalize();

			right = forward * orient;
			right.normalize();
			up = right * forward;
			up.normalize();
		}
		else {
			//	without arc-length
			Pnt3f train_cp_pos_p1 = m_pTrack->points[(int)t_time].pos;
			Pnt3f train_cp_pos_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].pos;

			Pnt3f train_cp_orient_p1 = m_pTrack->points[(int)t_time].orient;
			Pnt3f train_cp_orient_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].orient;

			float train_percentage = t_time - (int)t_time;
			qt = train_cp_pos_p2 * train_percentage + train_cp_pos_p1 * (1 - train_percentage);
			Pnt3f qt1 = train_cp_pos_p2 * (train_percentage + 0.0001) + train_cp_pos_p1 * (1 - train_percentage - 0.0001);

			forward = qt1 - qt;
			forward.normalize();
			Pnt3f orient = train_cp_orient_p2 * (train_percentage) + train_cp_orient_p1 * (1- train_percentage);
			orient.normalize();

			right = forward * orient;
			right.normalize();
			up = right * forward;
			up.normalize();
			
		}
	}

	if (tw->splineBrowser->value() >= 2) {

		if (tw->arcLength->value()) {
			
			Pnt3f train_cp_pos_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p3 = m_pTrack->points[(local_start_point + 2) % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p4 = m_pTrack->points[(local_start_point + 3) % m_pTrack->points.size()].pos;

			
			Pnt3f train_cp_orient_p1 = m_pTrack->points[local_start_point % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p2 = m_pTrack->points[(local_start_point + 1) % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p3 = m_pTrack->points[(local_start_point + 2) % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p4 = m_pTrack->points[(local_start_point + 3) % m_pTrack->points.size()].orient;
			
		
			
			float train_percentage;
			if (local_start_point == 0) {
				train_percentage = local_current_length / copy_list_sum_track_length[0];
			}
			else {
				if (copy_list_sum_track_length.size() > local_start_point)
					train_percentage = (local_current_length - copy_list_sum_track_length[local_start_point - 1]) / (copy_list_sum_track_length[local_start_point] - copy_list_sum_track_length[local_start_point - 1]);
				else {
					std::cout << "copy: " << copy_list_sum_track_length.size()  << " local_start_point " << local_start_point << std::endl;
					train_percentage = 0;
				}
			}

			qt = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, train_percentage, tw->splineBrowser->value());
			Pnt3f qt1 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, train_percentage + 0.0001, tw->splineBrowser->value());

			Pnt3f orient = GMT(train_cp_orient_p1, train_cp_orient_p2, train_cp_orient_p3, train_cp_orient_p4, train_percentage, tw->splineBrowser->value());

			forward = qt1 - qt;

			forward.normalize();
			orient.normalize();

			right = forward * orient;
			right.normalize();
			up = right * forward;
			up.normalize();
			
			
		}
		else {
			Pnt3f train_cp_pos_p1 = m_pTrack->points[(int)t_time].pos;
			Pnt3f train_cp_pos_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p3 = m_pTrack->points[((int)t_time + 2) % m_pTrack->points.size()].pos;
			Pnt3f train_cp_pos_p4 = m_pTrack->points[((int)t_time + 3) % m_pTrack->points.size()].pos;

			Pnt3f train_cp_orient_p1 = m_pTrack->points[(int)t_time].orient;
			Pnt3f train_cp_orient_p2 = m_pTrack->points[((int)t_time + 1) % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p3 = m_pTrack->points[((int)t_time + 2) % m_pTrack->points.size()].orient;
			Pnt3f train_cp_orient_p4 = m_pTrack->points[((int)t_time + 3) % m_pTrack->points.size()].orient;

			qt = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, t_time - (int)t_time, tw->splineBrowser->value());
			Pnt3f qt1 = GMT(train_cp_pos_p1, train_cp_pos_p2, train_cp_pos_p3, train_cp_pos_p4, t_time - (int)t_time + 0.0001, tw->splineBrowser->value());

			Pnt3f orient = GMT(train_cp_orient_p1, train_cp_orient_p2, train_cp_orient_p3, train_cp_orient_p4, t_time - (int)t_time, tw->splineBrowser->value());

			forward = qt1 - qt;

			forward.normalize();
			orient.normalize();

			right = forward * orient;
			right.normalize();
			up = right * forward;
			up.normalize();

		}
	}

	float rotation[16] = {
	forward.x, forward.y, forward.z, 0.0,
	up.x, up.y, up.z, 0.0,
	right.x, right.y, right.z, 0.0,
	0.0, 0.0, 0.0, 1.0
	};

	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	glTranslatef(-0.5f, -0.5f, -0.5f);
	drawCube(doingShadows);
	glPopMatrix();
 
	/*
	glBegin(GL_QUADS); //front
	if (!doingShadows)
		glColor3ub(220, 32, 64);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x + up.x, qt.y + forward.y + right.y + up.y, qt.z + forward.z + right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x + up.x, qt.y + forward.y - right.y + up.y, qt.z + forward.z - right.z + up.z);
	glEnd();

	glBegin(GL_QUADS); //right
	if (!doingShadows)
		glColor3ub(32, 220, 64);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x + up.x, qt.y - forward.y + right.y + up.y, qt.z - forward.z + right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x + up.x, qt.y + forward.y + right.y + up.y, qt.z + forward.z + right.z + up.z);
	glEnd();


	glBegin(GL_QUADS); //left
	if (!doingShadows)
		glColor3ub(32, 32, 220);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x + up.x, qt.y + forward.y - right.y + up.y, qt.z + forward.z - right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x + up.x, qt.y - forward.y - right.y + up.y, qt.z - forward.z - right.z + up.z);
	glEnd();

	glBegin(GL_QUADS); //back
	if (!doingShadows)
		glColor3ub(220, 220, 64);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x + up.x, qt.y - forward.y - right.y + up.y, qt.z - forward.z - right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x + up.x, qt.y - forward.y + right.y + up.y, qt.z - forward.z + right.z + up.z);
	glEnd();

	glBegin(GL_QUADS); //bottom
	if (!doingShadows)
		glColor3ub(140, 77, 211);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x + up.x, qt.y - forward.y - right.y + up.y, qt.z - forward.z - right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x + up.x, qt.y - forward.y + right.y + up.y, qt.z - forward.z + right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x + up.x, qt.y + forward.y + right.y + up.y, qt.z + forward.z + right.z + up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x + up.x, qt.y + forward.y - right.y + up.y, qt.z + forward.z - right.z + up.z);
	glEnd();

	/*
	glBegin(GL_QUADS); //top
	if (!doingShadows)
		glColor3ub(220, 220, 220);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
	glEnd();
	*/
	
}



void TrainView::
drawPlane(float* qt) {

	/*
	glBegin(GL_QUADS);					// D C
	// A B		
	glTexCoord2f(0.0f, 0.0f); //A	
	glVertex3f(qt.x + mainDir.x - side1.x - side2.x, qt.y + mainDir.y - side1.y - side2.y, qt.z + mainDir.z - side1.z - side2.z);
	glTexCoord2f(1.0f, 0.0f); //B
	glVertex3f(qt.x + mainDir.x + side1.x - side2.x, qt.y + mainDir.y + side1.y - side2.y, qt.z + mainDir.z + side1.z - side2.z);
	glTexCoord2f(1.0f, 1.0f); //C
	glVertex3f(qt.x + mainDir.x + side1.x + side2.x, qt.y + mainDir.y + side1.y + side2.y, qt.z + mainDir.z + side1.z + side2.z);
	glTexCoord2f(0.0f, 1.0f); //D
	glVertex3f(qt.x + mainDir.x - side1.x + side2.x, qt.y + mainDir.y - side1.y + side2.y, qt.z + mainDir.z - side1.z + side2.z);

	glEnd();
	*/
}



// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();
	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}