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

	// top view only needs one light
	/*
	if (tw->topCam->value()) {
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHT2);
	}
	*/

	//*********************************************************************
	//
	// * set the light parameters
	//
	//**********************************************************************
	
	
	

	GLfloat blackLight[]		= { 0.0f, 0.0f, 0.0f, 0.0f };

	
	//glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
	//glLightfv(GL_LIGHT0, GL_DIFFUSE, blackLight);
	
	GLfloat lightPosition0[] = { 0,100,0,0 }; // {50, 200.0, 50, 1.0};
	GLfloat lightPosition1[] = { -20, 15, 20, 1.0 };
	GLfloat lightPosition2[] = { 0, 2, 0, 1.0 };

	GLfloat spot_dir[] = { 0.0, 0.0, 1.0 };



	GLfloat yellowLight[] = { 0.5f, 0.5f, .1f, 1.0 };
	GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0 };
	GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
	GLfloat grayLight[] = { .3f, .3f, .3f, 1.0 };
	GLfloat redLight[] = { 1.0f, 0.0f, 0.0f, 1.0f };


	//glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	//glLightfv(GL_LIGHT1, GL_DIFFUSE, redLight);


	//glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	//glLightfv(GL_LIGHT2, GL_DIFFUSE, yellowLight);
	//glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, spot_dir);
	//glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 30.0f);


	
	
	/*
	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

	glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);
	*/

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	//*********************************************************************
	// now draw the ground plane
	//*********************************************************************
	// set to opengl fixed pipeline(use opengl 1.x draw function)
	glUseProgram(0);

	setupFloor();
	//glDisable(GL_LIGHTING);
	drawFloor(200,200);

	

	//*********************************************************************
	// now draw the object and we need to do it twice
	// once for real, and then once for shadows
	//*********************************************************************
	
	setupObjects();

	drawStuff();

	

	// this time drawing is for shadows (except for top view)
	if (!tw->topCam->value()) {
		setupShadows();
		drawStuff(true);
		unsetupShadows();
	}

	
	GLfloat headLight_pos[] = {current_train_pos.x, current_train_pos.y, current_train_pos.z, 1};
	GLfloat headLight_forward[] = { current_train_forward.x, current_train_forward.y, current_train_forward.z };
 
	glLightfv(GL_LIGHT0, GL_POSITION, headLight_pos);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, yellowLight);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, headLight_forward);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 30.0f);;


	glLightfv(GL_LIGHT1, GL_POSITION, lightPosition1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, whiteLight);


	
	
	glPushMatrix();
	glTranslatef(-20, 19, 20);
	glColor3f(1, 1, 1);
	gluSphere(gluNewQuadric(), 1, 100, 20);
	glPopMatrix();

	


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

void draw_one_plane(int start_x, int start_y, int end_x, int end_y, int num, int lock_dir, int lock_pos) {
	float push_x = (end_x - start_x) / num;
	float push_y = (end_y - start_y) / num;

	if (lock_dir == 2) {
		for (int i = 0; i < num; i++) {
			for (int j = 0; j < num; j++) {
				glBegin(GL_POLYGON);
				glVertex3f(start_x + push_x * i, start_y + push_y * j, lock_pos);
				glVertex3f(start_x + push_x * (i + 1), start_y + push_y * j, lock_pos);
				glVertex3f(start_x + push_x * (i + 1), start_y + push_y * (j + 1), lock_pos);
				glVertex3f(start_x + push_x * i, start_y + push_y * (j + 1), lock_pos);
				glEnd();
			}
		}
	}

	if (lock_dir == 0) {
		for (int i = 0; i < num; i++) {
			for (int j = 0; j < num; j++) {
				glBegin(GL_POLYGON);
				glVertex3f(lock_pos, start_x + push_x * i, start_y + push_y * j);
				glVertex3f(lock_pos, start_x + push_x * (i + 1), start_y + push_y * j);
				glVertex3f(lock_pos, start_x + push_x * (i + 1), start_y + push_y * (j + 1));
				glVertex3f(lock_pos, start_x + push_x * i, start_y + push_y * (j + 1));
				glEnd();
			}
		}
	}
}

void TrainView::drawStuff(bool doingShadows)
{
	std::vector<float> list_sum_track_length;

	


	

	// my_scene
	if (tw->my_scene->value()) {
		
		

		glBegin(GL_LINES);
		if (!doingShadows)
			glColor3f(1, 1, 1);
		glVertex3f(-25, 0, 25);
		glVertex3f(-25, 20, 25);
		
		glEnd();

		glBegin(GL_LINES);
		if (!doingShadows)
			glColor3f(1, 1, 1);
		glVertex3f(-25, 20, 25);
		glVertex3f(-20, 20, 20);

		glEnd();
		
		
		

		if (!doingShadows)
			glColor3f(0.8,0.8,0.8);
		draw_one_plane(20, 0, 80, 20, 10, 2, -20); //(x,y)
		draw_one_plane(20, 20, 40, 40, 10, 2, -20);
		draw_one_plane(60, 20, 80, 40, 10, 2, -20);
		draw_one_plane(20, 40, 80, 100, 10, 2, -20);

		

		draw_one_plane(0, -20, 60, -80, 10, 0, 20); //(y,z)
		draw_one_plane(60, -20, 80, -40, 10, 0, 20);
		draw_one_plane(60, -60, 80, -80, 10, 0, 20);
		draw_one_plane(80, -20, 100, -80, 10, 0, 20);

		if (!doingShadows)
			glColor3f(0.2, 0.2, 0.2);
		draw_one_plane(20, 0, 80, 100, 10, 2, -80);
		draw_one_plane(0, -20, 100, -80, 10, 0, 80);
	
	}
	

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
	float tile_arc_two_cp_length = 0;
	std::vector<Pnt3f> arc_list_tile_qt;

	float arc_tunnel_two_cp_length = 0;
	std::vector<Pnt3f> arc_list_tunnel_qt;
	
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
				tile_arc_two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));

				if (!tw->rail_parallel->value()) {
					glLineWidth(3);
					glBegin(GL_LINES);
					if (!doingShadows)
						glColor3ub(32, 32, 64);
					glVertex3f(qt0.x, qt0.y, qt0.z);
					glVertex3f(qt1.x, qt1.y, qt1.z);
					glEnd();
				}
				// cross


				Pnt3f orient_t = (1 - t) * cp_orient_p1 + t * cp_orient_p2;
				orient_t.normalize();
				// Pnt3f forward = 
				Pnt3f forward = (qt1 - qt0);
				forward.normalize();
				Pnt3f cross_t = (qt1 - qt0) * orient_t;

				cross_t.normalize();
				cross_t = cross_t * 2.5f;
				if (tw->rail_parallel->value()) {
					glBegin(GL_LINES);
					if (!doingShadows)
						glColor3f(1,0,0);
					glLineWidth(3);
					glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
					glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
					glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
					glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
					glEnd();
				}

				
				if (tile_arc_two_cp_length >= 10) {
					arc_list_tile_qt.push_back(qt);
					arc_list_tile_qt.push_back(cross_t);
					arc_list_tile_qt.push_back(forward);
					tile_arc_two_cp_length = 0;

				}

				arc_list_tunnel_qt.push_back(qt);
				arc_list_tunnel_qt.push_back(cross_t);
				arc_list_tunnel_qt.push_back(forward);
				


				



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
		int length_arc_list_tile_qt = arc_list_tile_qt.size();
		int length_arc_list_tunnel_qt = (int)(arc_list_tunnel_qt.size() * tw->tunnel_length->value());

		// draw tile
		if (tw->rail_tile->value()) {
			
			for (int i = 0; i < length_arc_list_tile_qt; i += 3) {
				Pnt3f qt = arc_list_tile_qt[i];
				Pnt3f right = arc_list_tile_qt[i + 1] * 2;
				Pnt3f forward = arc_list_tile_qt[i + 2] * 2;
				Pnt3f up = right * forward;
				up.normalize();

				if (!doingShadows)
					glColor3f(1, 0, 0);
				
				// up
				glBegin(GL_POLYGON);
				glNormal3f(up.x, up.y, up.z);
				glVertex3f(qt.x + forward.x - right.x, qt.y + forward.y - right.y, qt.z + forward.z - right.z);
				glVertex3f(qt.x + forward.x + right.x, qt.y + forward.y + right.y, qt.z + forward.z + right.z);
				glVertex3f(qt.x - forward.x + right.x, qt.y - forward.y + right.y, qt.z - forward.z + right.z);
				glVertex3f(qt.x - forward.x - right.x, qt.y - forward.y - right.y, qt.z - forward.z - right.z);
				glEnd();

				//down
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
				glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
				glEnd();

				//left
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x + right.x, qt.y + forward.y + right.y, qt.z + forward.z + right.z);
				glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x, qt.y - forward.y + right.y, qt.z - forward.z + right.z);
				glEnd();

				//left
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x - right.x, qt.y + forward.y - right.y, qt.z + forward.z - right.z);
				glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x, qt.y - forward.y - right.y, qt.z - forward.z - right.z);
				glEnd();

			}
		}

		if (tw->rail_tunnel->value()) {

			for (int i = 0; i < length_arc_list_tunnel_qt; i += 3) {
				Pnt3f qt = arc_list_tunnel_qt[i];
				Pnt3f right = arc_list_tunnel_qt[i + 1] * 3;
				Pnt3f forward = arc_list_tunnel_qt[i + 2] * 3;
				Pnt3f up = right * forward;

				forward.normalize();
				forward = forward * 0.1;

				up.normalize();
				up = up * 10;

				if (!doingShadows) {
					glColor3f(0.5, 0.5, 0.1);
				}

				//right
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + right.x, qt.y + right.y, qt.z + right.z);
				glVertex3f(qt.x + right.x + up.x, qt.y + right.y + up.y, qt.z + right.z + up.z);
				glVertex3f(qt.x + right.x * 1.2 + up.x, qt.y + right.y * 1.2 + up.y, qt.z + right.z * 1.2 + up.z);
				glVertex3f(qt.x + right.x * 1.2, qt.y + right.y * 1.2, qt.z + right.z * 1.2);
				glEnd();

				// top 
				glBegin(GL_POLYGON);
				glVertex3f(qt.x + up.x - right.x * 1.2, qt.y + up.y - right.y * 1.2, qt.z + up.z - right.z * 1.2);
				glVertex3f(qt.x + up.x + right.x * 1.2, qt.y + up.y + right.y * 1.2, qt.z + up.z + right.z * 1.2);
				glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2, qt.y + up.y * 1.1 + right.y * 1.2, qt.z + up.z * 1.1 + right.z * 1.2);
				glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2, qt.y + up.y * 1.1 - right.y * 1.2, qt.z + up.z * 1.1 - right.z * 1.2);
				glEnd();

				// top outside
				glBegin(GL_POLYGON);
				glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2, qt.y + up.y * 1.1 + right.y * 1.2, qt.z + up.z * 1.1 + right.z * 1.2);
				glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2, qt.y + up.y * 1.1 - right.y * 1.2, qt.z + up.z * 1.1 - right.z * 1.2);
				glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2 + forward.x, qt.y + up.y * 1.1 - right.y * 1.2 + forward.y, qt.z + up.z * 1.1 - right.z * 1.2 + forward.z);
				glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2 + forward.x, qt.y + up.y * 1.1 + right.y * 1.2 + forward.y, qt.z + up.z * 1.1 + right.z * 1.2 + forward.z);
				glEnd();

				// top inside
				glBegin(GL_POLYGON);
				glVertex3f(qt.x + up.x + right.x * 1.2, qt.y + up.y  + right.y * 1.2, qt.z + up.z  + right.z * 1.2);
				glVertex3f(qt.x + up.x  - right.x * 1.2, qt.y + up.y  - right.y * 1.2, qt.z + up.z  - right.z * 1.2);
				glVertex3f(qt.x + up.x  - right.x * 1.2 + forward.x, qt.y + up.y  - right.y * 1.2 + forward.y, qt.z + up.z  - right.z * 1.2 + forward.z);
				glVertex3f(qt.x + up.x  + right.x * 1.2 + forward.x, qt.y + up.y  + right.y * 1.2 + forward.y, qt.z + up.z  + right.z * 1.2 + forward.z);
				glEnd();




				//right outside
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + right.x * 1.2, qt.y + right.y * 1.2, qt.z + right.z * 1.2);
				glVertex3f(qt.x + right.x * 1.2 + up.x, qt.y + right.y * 1.2 + up.y, qt.z + right.z * 1.2 + up.z);
				glVertex3f(qt.x + right.x * 1.2 + up.x + forward.x, qt.y + right.y * 1.2 + up.y + forward.y, qt.z + right.z * 1.2 + up.z + forward.z);
				glVertex3f(qt.x + right.x * 1.2 + forward.x, qt.y + right.y * 1.2 + forward.y, qt.z + right.z * 1.2 + forward.z);
				glEnd();

				//right inside
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + right.x , qt.y + right.y , qt.z + right.z );
				glVertex3f(qt.x + right.x  + up.x, qt.y + right.y  + up.y, qt.z + right.z  + up.z);
				glVertex3f(qt.x + right.x  + up.x + forward.x, qt.y + right.y  + up.y + forward.y, qt.z + right.z  + up.z + forward.z);
				glVertex3f(qt.x + right.x  + forward.x, qt.y + right.y  + forward.y, qt.z + right.z  + forward.z);
				glEnd();

				//left
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x - right.x, qt.y - right.y, qt.z - right.z);
				glVertex3f(qt.x - right.x + up.x, qt.y - right.y + up.y, qt.z - right.z + up.z);
				glVertex3f(qt.x - right.x * 1.2 + up.x, qt.y - right.y * 1.2 + up.y, qt.z - right.z * 1.2 + up.z);
				glVertex3f(qt.x - right.x * 1.2, qt.y - right.y * 1.2, qt.z - right.z * 1.2);
				glEnd();

				//left outside
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x - right.x * 1.2, qt.y - right.y * 1.2, qt.z - right.z * 1.2);
				glVertex3f(qt.x - right.x * 1.2 + up.x, qt.y - right.y * 1.2 + up.y, qt.z - right.z * 1.2 + up.z);
				glVertex3f(qt.x - right.x * 1.2 + up.x + forward.x, qt.y - right.y * 1.2 + up.y + forward.y, qt.z - right.z * 1.2 + up.z + forward.z);
				glVertex3f(qt.x - right.x * 1.2 + forward.x, qt.y - right.y * 1.2 + forward.y, qt.z - right.z * 1.2 + forward.z);
				glEnd();

				//left inside
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x - right.x , qt.y - right.y , qt.z - right.z );
				glVertex3f(qt.x - right.x  + up.x, qt.y - right.y  + up.y, qt.z - right.z  + up.z);
				glVertex3f(qt.x - right.x  + up.x + forward.x, qt.y - right.y  + up.y + forward.y, qt.z - right.z  + up.z + forward.z);
				glVertex3f(qt.x - right.x  + forward.x, qt.y - right.y  + forward.y, qt.z - right.z  + forward.z);
				glEnd();
			}
		}

		if (tw->rail_support->value()) {

			for (int i = 0; i < length_arc_list_tile_qt; i += 6) {
				Pnt3f qt = arc_list_tile_qt[i];
				Pnt3f right = arc_list_tile_qt[i + 1];
				Pnt3f forward = arc_list_tile_qt[i + 2] * 2;
				Pnt3f up = right * forward;
				up.normalize();

				if (!tw->rail_parallel->value()) {
					// up
					if (!doingShadows)
						glColor3f(1, 0, 0);
					glBegin(GL_LINES);
					glLineWidth(200);
					glVertex3f(qt.x, qt.y, qt.z);
					glVertex3f(qt.x, 0, qt.z);
					glEnd();
				}
				else {
					if (!doingShadows)
						glColor3f(1, 0, 0);
					glBegin(GL_LINES);
					glLineWidth(200);
					glVertex3f(qt.x + right.x, qt.y + right.y, qt.z + right.z);
					glVertex3f(qt.x + right.x, 0, qt.z + right.z);

					glVertex3f(qt.x - right.x, qt.y - right.y, qt.z - right.z);
					glVertex3f(qt.x - right.x, 0, qt.z - right.z);
					glEnd();
				}


			}
		}
		
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
			float arc_two_cp_length = 0;

			for (size_t j = 0; j < DIVIDE_LINE; j++) {
				Pnt3f qt0 = qt;
				t += percent;
				qt = GMT(cp_pos_p1, cp_pos_p2, cp_pos_p3, cp_pos_p4, t, tw->splineBrowser->value());
				Pnt3f qt1 = qt;

				two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));
				
				arc_two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));
				tile_arc_two_cp_length += std::sqrt((qt1.x - qt0.x) * (qt1.x - qt0.x) + (qt1.y - qt0.y) * (qt1.y - qt0.y) + (qt1.z - qt0.z) * (qt1.z - qt0.z));

				

				if (arc_two_cp_length >= 1) {
					list_qt.push_back(qt);
					arc_two_cp_length = 0;
				}




				if (!tw->rail_parallel->value()) {
					glLineWidth(3);
					glBegin(GL_LINES);
					if (!doingShadows)
						glColor3ub(1, 0, 0);
					glVertex3f(qt0.x, qt0.y, qt0.z);
					glVertex3f(qt1.x, qt1.y, qt1.z);
					glEnd();
				}

				Pnt3f orient_t = GMT(cp_orient_p1, cp_orient_p2, cp_orient_p3, cp_orient_p4, t, tw->splineBrowser->value());
				orient_t.normalize();
				Pnt3f forward = qt1 - qt0;
				forward.normalize();

				Pnt3f cross_t = (qt1 - qt0) * orient_t;

				cross_t.normalize();
				cross_t = cross_t * 2.5f;
				if (tw->rail_parallel->value()) {
					if (!doingShadows)
						glColor3f(1, 0, 0);
					glBegin(GL_LINES);
					glLineWidth(3);
					glVertex3f(qt0.x + cross_t.x, qt0.y + cross_t.y, qt0.z + cross_t.z);
					glVertex3f(qt1.x + cross_t.x, qt1.y + cross_t.y, qt1.z + cross_t.z);
					glVertex3f(qt0.x - cross_t.x, qt0.y - cross_t.y, qt0.z - cross_t.z);
					glVertex3f(qt1.x - cross_t.x, qt1.y - cross_t.y, qt1.z - cross_t.z);
					glEnd();
				}

				if (tile_arc_two_cp_length >= 10) {
					arc_list_tile_qt.push_back(qt);
					arc_list_tile_qt.push_back(cross_t);
					arc_list_tile_qt.push_back(forward);
					tile_arc_two_cp_length = 0;

				}

				arc_list_tunnel_qt.push_back(qt0);
				arc_list_tunnel_qt.push_back(cross_t);
				arc_list_tunnel_qt.push_back(forward);

			}
			
			total_length += two_cp_length;
			list_track_length.push_back(two_cp_length);
			list_sum_track_length.push_back(total_length);
			
		}
		
		int length_list_track_length = list_track_length.size();
		tw->tv_length_list_track_length = length_list_track_length;
		tw->tv_list_track_length = list_track_length;

		copy_list_sum_track_length = list_sum_track_length;
		int length_list_qt = list_qt.size();
		//std::cout << length_list_qt << std::endl;
		copy_list_qt = list_qt;

		for (int my_i = 0; my_i < length_list_track_length; my_i++) {
			list_track_length.pop_back();
			list_sum_track_length.pop_back();
		}
		for (int my_i = 0; my_i < length_list_qt; my_i++) {
			list_qt.pop_back();
		}


		int length_arc_list_tile_qt = arc_list_tile_qt.size();
		if (tw->rail_tile->value()) {

			for (int i = 0; i < length_arc_list_tile_qt; i += 3) {
				Pnt3f qt = arc_list_tile_qt[i];
				Pnt3f right = arc_list_tile_qt[i + 1] * 2;
				Pnt3f forward = arc_list_tile_qt[i + 2] * 2;
				Pnt3f up = right * forward;
				up.normalize();

				if (!doingShadows)
					glColor3f(1, 0, 0);

				//up 
				glBegin(GL_POLYGON);
				glNormal3f(up.x, up.y, up.z);
				glVertex3f(qt.x + forward.x - right.x, qt.y + forward.y - right.y, qt.z + forward.z - right.z);
				glVertex3f(qt.x + forward.x + right.x, qt.y + forward.y + right.y, qt.z + forward.z + right.z);
				glVertex3f(qt.x - forward.x + right.x, qt.y - forward.y + right.y, qt.z - forward.z + right.z);
				glVertex3f(qt.x - forward.x - right.x, qt.y - forward.y - right.y, qt.z - forward.z - right.z);
				glEnd();

				//down
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
				glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
				glEnd();

				//left
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x + right.x, qt.y + forward.y + right.y, qt.z + forward.z + right.z);
				glVertex3f(qt.x + forward.x + right.x - up.x, qt.y + forward.y + right.y - up.y, qt.z + forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x - up.x, qt.y - forward.y + right.y - up.y, qt.z - forward.z + right.z - up.z);
				glVertex3f(qt.x - forward.x + right.x, qt.y - forward.y + right.y, qt.z - forward.z + right.z);
				glEnd();

				//left
				glBegin(GL_POLYGON);
				glNormal3f(-up.x, -up.y, -up.z);
				glVertex3f(qt.x + forward.x - right.x, qt.y + forward.y - right.y, qt.z + forward.z - right.z);
				glVertex3f(qt.x + forward.x - right.x - up.x, qt.y + forward.y - right.y - up.y, qt.z + forward.z - right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x - up.x, qt.y - forward.y - right.y - up.y, qt.z - forward.z - right.z - up.z);
				glVertex3f(qt.x - forward.x - right.x, qt.y - forward.y - right.y, qt.z - forward.z - right.z);
				glEnd();
			}
		}

		if (tw->rail_support->value()) {

			for (int i = 0; i < length_arc_list_tile_qt; i += 6) {
				Pnt3f qt = arc_list_tile_qt[i];
				Pnt3f right = arc_list_tile_qt[i + 1];
				Pnt3f forward = arc_list_tile_qt[i + 2] * 2;
				Pnt3f up = right * forward;
				up.normalize();

				if (!tw->rail_parallel->value()) {
					// up
					if (!doingShadows)
						glColor3f(1, 0, 0);
					glBegin(GL_LINES);
					glLineWidth(200);
					glVertex3f(qt.x, qt.y, qt.z);
					glVertex3f(qt.x, 0, qt.z);
					glEnd();
				}
				else {
					if (!doingShadows)
						glColor3f(1, 0, 0);
					glBegin(GL_LINES);
					glLineWidth(200);
					glVertex3f(qt.x + right.x, qt.y + right.y, qt.z + right.z);
					glVertex3f(qt.x + right.x, 0, qt.z + right.z);

					glVertex3f(qt.x - right.x, qt.y - right.y, qt.z - right.z);
					glVertex3f(qt.x - right.x, 0, qt.z - right.z);
					glEnd();
				}


			}
		}

		int length_arc_list_tunnel_qt = (int)(arc_list_tunnel_qt.size() * tw->tunnel_length->value());
		//std::cout << "tunnel length: " << length_arc_list_tunnel_qt << std::endl;
		if (tw->rail_tunnel->value()) {
			//std::cout << "draw_tunnel" << std::endl;

			if (length_arc_list_tunnel_qt > 0) {
				for (int i = 0; i < length_arc_list_tunnel_qt; i += 3) {
					Pnt3f qt = arc_list_tunnel_qt[i];
					Pnt3f right = arc_list_tunnel_qt[i + 1] * 3;
					Pnt3f forward = arc_list_tunnel_qt[i + 2] * 3;
					Pnt3f up = right * forward;

					forward.normalize();
					forward = forward * 0.1;

					up.normalize();
					up = up * 10;

					if (!doingShadows) {
						glColor3f(0.5, 0.5, 0.1);
					}

					//right
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x + right.x, qt.y + right.y, qt.z + right.z);
					glVertex3f(qt.x + right.x + up.x, qt.y + right.y + up.y, qt.z + right.z + up.z);
					glVertex3f(qt.x + right.x * 1.2 + up.x, qt.y + right.y * 1.2 + up.y, qt.z + right.z * 1.2 + up.z);
					glVertex3f(qt.x + right.x * 1.2, qt.y + right.y * 1.2, qt.z + right.z * 1.2);
					glEnd();

					// top 
					glBegin(GL_POLYGON);
					glVertex3f(qt.x + up.x - right.x * 1.2, qt.y + up.y - right.y * 1.2, qt.z + up.z - right.z * 1.2);
					glVertex3f(qt.x + up.x + right.x * 1.2, qt.y + up.y + right.y * 1.2, qt.z + up.z + right.z * 1.2);
					glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2, qt.y + up.y * 1.1 + right.y * 1.2, qt.z + up.z * 1.1 + right.z * 1.2);
					glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2, qt.y + up.y * 1.1 - right.y * 1.2, qt.z + up.z * 1.1 - right.z * 1.2);
					glEnd();

					// top outside
					glBegin(GL_POLYGON);
					glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2, qt.y + up.y * 1.1 + right.y * 1.2, qt.z + up.z * 1.1 + right.z * 1.2);
					glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2, qt.y + up.y * 1.1 - right.y * 1.2, qt.z + up.z * 1.1 - right.z * 1.2);
					glVertex3f(qt.x + up.x * 1.1 - right.x * 1.2 + forward.x, qt.y + up.y * 1.1 - right.y * 1.2 + forward.y, qt.z + up.z * 1.1 - right.z * 1.2 + forward.z);
					glVertex3f(qt.x + up.x * 1.1 + right.x * 1.2 + forward.x, qt.y + up.y * 1.1 + right.y * 1.2 + forward.y, qt.z + up.z * 1.1 + right.z * 1.2 + forward.z);
					glEnd();

					// top inside
					glBegin(GL_POLYGON);
					glVertex3f(qt.x + up.x + right.x * 1.2, qt.y + up.y + right.y * 1.2, qt.z + up.z + right.z * 1.2);
					glVertex3f(qt.x + up.x - right.x * 1.2, qt.y + up.y - right.y * 1.2, qt.z + up.z - right.z * 1.2);
					glVertex3f(qt.x + up.x - right.x * 1.2 + forward.x, qt.y + up.y - right.y * 1.2 + forward.y, qt.z + up.z - right.z * 1.2 + forward.z);
					glVertex3f(qt.x + up.x + right.x * 1.2 + forward.x, qt.y + up.y + right.y * 1.2 + forward.y, qt.z + up.z + right.z * 1.2 + forward.z);
					glEnd();




					// top inside

					//right outside
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x + right.x * 1.2, qt.y + right.y * 1.2, qt.z + right.z * 1.2);
					glVertex3f(qt.x + right.x * 1.2 + up.x, qt.y + right.y * 1.2 + up.y, qt.z + right.z * 1.2 + up.z);
					glVertex3f(qt.x + right.x * 1.2 + up.x + forward.x, qt.y + right.y * 1.2 + up.y + forward.y, qt.z + right.z * 1.2 + up.z + forward.z);
					glVertex3f(qt.x + right.x * 1.2 + forward.x, qt.y + right.y * 1.2 + forward.y, qt.z + right.z * 1.2 + forward.z);
					glEnd();

					//right inside
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x + right.x, qt.y + right.y, qt.z + right.z);
					glVertex3f(qt.x + right.x + up.x, qt.y + right.y + up.y, qt.z + right.z + up.z);
					glVertex3f(qt.x + right.x + up.x + forward.x, qt.y + right.y + up.y + forward.y, qt.z + right.z + up.z + forward.z);
					glVertex3f(qt.x + right.x + forward.x, qt.y + right.y + forward.y, qt.z + right.z + forward.z);
					glEnd();

					//left
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x - right.x, qt.y - right.y, qt.z - right.z);
					glVertex3f(qt.x - right.x + up.x, qt.y - right.y + up.y, qt.z - right.z + up.z);
					glVertex3f(qt.x - right.x * 1.2 + up.x, qt.y - right.y * 1.2 + up.y, qt.z - right.z * 1.2 + up.z);
					glVertex3f(qt.x - right.x * 1.2, qt.y - right.y * 1.2, qt.z - right.z * 1.2);
					glEnd();

					//left outside
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x - right.x * 1.2, qt.y - right.y * 1.2, qt.z - right.z * 1.2);
					glVertex3f(qt.x - right.x * 1.2 + up.x, qt.y - right.y * 1.2 + up.y, qt.z - right.z * 1.2 + up.z);
					glVertex3f(qt.x - right.x * 1.2 + up.x + forward.x, qt.y - right.y * 1.2 + up.y + forward.y, qt.z - right.z * 1.2 + up.z + forward.z);
					glVertex3f(qt.x - right.x * 1.2 + forward.x, qt.y - right.y * 1.2 + forward.y, qt.z - right.z * 1.2 + forward.z);
					glEnd();

					//left inside
					glBegin(GL_POLYGON);
					glNormal3f(-up.x, -up.y, -up.z);
					glVertex3f(qt.x - right.x, qt.y - right.y, qt.z - right.z);
					glVertex3f(qt.x - right.x + up.x, qt.y - right.y + up.y, qt.z - right.z + up.z);
					glVertex3f(qt.x - right.x + up.x + forward.x, qt.y - right.y + up.y + forward.y, qt.z - right.z + up.z + forward.z);
					glVertex3f(qt.x - right.x + forward.x, qt.y - right.y + forward.y, qt.z - right.z + forward.z);
					glEnd();
				}
			}
		}
	}

	if (!tw->trainCam->value()) {
		drawTrain(doingShadows, 0, 1);
		for (int i = 0; i < num_cars; i++) {
			drawTrain(doingShadows, (i + 1) * 10, 0);
		}
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


Pnt3f TrainView::GMT(const Pnt3f& pt0, const Pnt3f& pt1, const Pnt3f& pt2, const Pnt3f& pt3, double t, int type) {
	glm::mat4x4 M;
	switch (type) {
	case 2: {
		float s = tw->tension->value();
		M = {
			-s, 2*s, -s, 0,
			2-s, s-3, 0, 1,
			s-2, 3-2*s, s, 0,
			s, -s, 0, 0
		};
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
	{0.5, 0.0, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.0, 0.5},
	{0.5, 0.0, 0.5}, {0.5, 0.0, 0.5}, {0.5, 0.0, 0.5}
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

void TrainView::drawTrain(bool doingShadows, float backward_distance, bool head) {
	Pnt3f qt;
	Pnt3f forward;
	Pnt3f right;
	Pnt3f up;
	// set start_point for arc-length
	
	float local_current_length = current_length - backward_distance;

	int local_start_point = 0;
	
	if (local_current_length < 0) {
		local_current_length += copy_list_qt.size() - 1;
	}
	if (!head) {
		//std::cout << "passenger" << local_current_length << std::endl;
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
			
		
			
			double train_percentage;
			if (local_start_point == 0) {
				train_percentage = local_current_length / copy_list_sum_track_length[0];
			}
			else {
				if (copy_list_sum_track_length.size() > local_start_point) {
					train_percentage = ((local_current_length - copy_list_sum_track_length[local_start_point - 1]) / (copy_list_sum_track_length[local_start_point] - copy_list_sum_track_length[local_start_point - 1]));
					//std::cout << "here" << std::endl;
					//std::cout << "train: " << train_percentage << std::endl;
				}
				else {
					std::cout << "copy: " << copy_list_sum_track_length.size()  << " local_start_point " << local_start_point << std::endl;
					train_percentage = 0;
				}
			}

			
			Pnt3f qt1;

			if (local_current_length < copy_list_qt.size() - 1) {
				qt = copy_list_qt[local_current_length];
				qt1 = copy_list_qt[local_current_length + 1];
			}
			else {
				//std::cout << local_current_length << std::endl;
				if (head)
					current_length = 0;
				qt = copy_list_qt[0];
				qt1 = copy_list_qt[1];
			}


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

	float rotation_90[16] = {
	cos(-1.5707963268), 0, sin(-1.5707963268), 0,
	0, 1, 0, 0,
	-sin(-1.5707963268), 0, cos(-1.5707963268), 0,
	0, 0, 0, 1
	};


	float deg = (local_current_length / ( 2 * 0.3 * 3.14)) * 360;
	float rad = (int(deg) % 360) * 3.14 / 180;

	// std::cout << "local_length: " << local_current_length << " deg: " << deg << std::endl;

	float rotation_wheel_y[16] = {
		cos(1), 0, sin(1), 0,
		0, 1, 0, 0,
		-sin(1), 0, cos(1), 0,
		0,0,0,1
	};

	float rotation_wheel_z[16] = {
		cos(rad), -sin(rad), 0, 0,
		sin(rad), cos(rad), 0, 0,
		0,0,1,0,
		0,0,0,1
	};

	if (head) {
		current_train_pos = qt;
		current_train_forward = forward;
	}
	
	
	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	glTranslatef(-0.5f, 1, -0.5f);
	if (head) {
		drawCube(doingShadows);
	}
	
	glMultMatrixf(rotation_90);
	glScalef(1.2f, 1.2f, 1.2f);
	glTranslatef(-0.5, 0, 0);
	if (head) {
		if (!doingShadows)
			glColor3f(0.5, 0, 0);
		gluCylinder(gluNewQuadric(), 0.5, 0.5, 2, 50, 1);
		if (!doingShadows)
			glColor3f(0.5, 0, 0.5);
		gluDisk(gluNewQuadric(), 0.0, 0.5, 64, 1);
	}

	//back splash
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(-0.5, -0.7, 0.1);
	glVertex3f(0.5, -0.7, 0.1);
	glVertex3f(0.5, -0.2, 0.1);
	glVertex3f(-0.5, -0.2, 0.1);
	glEnd();
	
	//left splash
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(0.5, -0.7, 0.1);
	glVertex3f(0.5, -0.2, 0.1);
	glVertex3f(0.5, -0.2, 1.5);
	glVertex3f(0.5, -0.7, 1.5);
	glEnd();

	// bottom splash
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(0.5, -0.7, 0.1);
	glVertex3f(0.5, -0.7, 1.5);
	glVertex3f(-0.5, -0.7, 1.5);
	glVertex3f(-0.5, -0.7, 0.1);
	glEnd();

	// top splash
	
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(0.5, -0.2, 0.1);
	glVertex3f(0.5, -0.2, 1.5);
	glVertex3f(-0.5, -0.2, 1.5);
	glVertex3f(-0.5, -0.2, 0.1);
	glEnd();
	

	//right splash
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(-0.5, -0.7, 0.1);
	glVertex3f(-0.5, -0.2, 0.1);
	glVertex3f(-0.5, -0.2, 1.5);
	glVertex3f(-0.5, -0.7, 1.5);
	glEnd();

	//front train
	glTranslatef(0,0,2);
	if (head) {
		if (!doingShadows)
			glColor3f(1, 1, 0);
		gluDisk(gluNewQuadric(), 0.0, 0.5, 64, 1);
	}


	//front splash
	glTranslatef(0, 0, -0.5);
	glTranslatef(0, -0.7, 0);
	glBegin(GL_POLYGON);
	if (!doingShadows)
		glColor3f(0, 0, 0);
	glVertex3f(-0.5, 0, 0);
	glVertex3f(0.5, 0, 0);
	glVertex3f(0.5, 0.5, 0);
	glVertex3f(-0.5, 0.5, 0);
	glEnd();


	glPopMatrix();


	// left hind wheel
	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	if (!doingShadows)
		glColor3f(1, 0, 0);
	
	glTranslatef(0,0,-0.7);
	glMultMatrixf(rotation_wheel_z);
	gluCylinder(gluNewQuadric(), 0.3, 0.3, 0.5, 50, 1);
	gluDisk(gluNewQuadric(), 0.0, 0.3, 64, 1);


	glTranslatef(0, 0, -0.05);
	
	glBegin(GL_LINES);
	glLineWidth(20);
	if (!doingShadows)
		glColor3f(0, 1, 0);
	glVertex3f(0, -0.3, 0);
	glVertex3f(0, 0.3, 0);
	glEnd();
	glPopMatrix();

	// left front wheel
	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	if (!doingShadows)
		glColor3f(1, 0, 0);

	glTranslatef(1, 0, -0.7);
	glMultMatrixf(rotation_wheel_z);
	gluCylinder(gluNewQuadric(), 0.3, 0.3, 0.5, 50, 1);
	gluDisk(gluNewQuadric(), 0.0, 0.3, 64, 1);


	glTranslatef(0, 0, -0.05);
	glBegin(GL_LINES);
	glLineWidth(20);
	if (!doingShadows)
		glColor3f(0, 1, 0);
	glVertex3f(0, -0.3, 0);
	glVertex3f(0, 0.3, 0);
	glEnd();
	glPopMatrix();

	// right hind wheel
	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	if (!doingShadows)
		glColor3f(1, 0, 0);

	glTranslatef(0, 0, 0.2);
	glMultMatrixf(rotation_wheel_z);
	gluCylinder(gluNewQuadric(), 0.3, 0.3, 0.5, 50, 1);
	glTranslatef(0, 0, 0.5);
	gluDisk(gluNewQuadric(), 0.0, 0.3, 64, 1);

	glTranslatef(0, 0, 0.05);
	glBegin(GL_LINES);
	glLineWidth(20);
	if (!doingShadows)
		glColor3f(0, 1, 0);
	glVertex3f(0, -0.3, 0);
	glVertex3f(0, 0.3, 0);
	glEnd();
	glPopMatrix();
	


	

	// right front wheel
	glPushMatrix();
	glTranslatef(qt.x, qt.y, qt.z);
	glMultMatrixf(rotation);
	glScalef(5.0f, 5.0f, 5.0f);
	if (!doingShadows)
		glColor3f(1, 0, 0);

	glTranslatef(1, 0, 0.2);
	glMultMatrixf(rotation_wheel_z);
	gluCylinder(gluNewQuadric(), 0.3, 0.3, 0.5, 50, 1);
	glTranslatef(0, 0, 0.5);
	gluDisk(gluNewQuadric(), 0.0, 0.3, 64, 1);


	glTranslatef(0, 0, 0.05);
	glBegin(GL_LINES);
	glLineWidth(20);
	if (!doingShadows)
		glColor3f(0, 1, 0);
	glVertex3f(0, -0.3, 0);
	glVertex3f(0, 0.3, 0);
	glEnd();
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