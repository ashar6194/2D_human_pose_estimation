#include "main.h"
#include "Glut.h"
#include <opencv2/opencv.hpp>
#include <opencv2\highgui\highgui.hpp>

#include "opencv2/core/core.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/features2d/features2d.hpp"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <Windows.h>
#include <Ole2.h> 

#include <Kinect.h>
//#define count1 0

using namespace cv;
int ccc = 0, c3 = 300, pt = 0, prec = 5; //ccc for counting till the 10th second, c3 is the reference, pt serves as a flag for the external camera.

// We'll be using buffer objects to store the kinect point cloud
GLuint vboId;
GLuint cboId;

// Intermediate Buffers
unsigned char rgbimage[colorwidth*colorheight*4];    // Stores RGB color image
ColorSpacePoint depth2rgb[width*height];             // Maps depth pixels to rgb pixels
CameraSpacePoint depth2xyz[width*height];			 // Maps depth pixels to 3d coordinates

// Body tracking variables
BOOLEAN tracked;							// Whether we see a body
Joint joints[JointType_Count];				// List of joints in the tracked body
Joint jointscap[JointType_Count];			// Positions of the captured coordinate;
// Kinect Variables
IKinectSensor* sensor;             // Kinect sensor
IMultiSourceFrameReader* reader;   // Kinect data source
ICoordinateMapper* mapper;         // Converts between depth, color, and 3d coordinates

bool initKinect() {
    if (FAILED(GetDefaultKinectSensor(&sensor))) {
		return false;
	}
	if (sensor) {
		sensor->get_CoordinateMapper(&mapper);

		sensor->Open();
		sensor->OpenMultiSourceFrameReader(
			FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color | FrameSourceTypes::FrameSourceTypes_Body,
			&reader);
		return reader;
	} else {
		return false;
	}
}

void getDepthData(IMultiSourceFrame* frame, GLubyte* dest) {
	IDepthFrame* depthframe;
	IDepthFrameReference* frameref = NULL;
	frame->get_DepthFrameReference(&frameref);
	frameref->AcquireFrame(&depthframe);
	if (frameref) frameref->Release();

	if (!depthframe) return;

	// Get data from frame
	unsigned int sz;
	unsigned short* buf;
	depthframe->AccessUnderlyingBuffer(&sz, &buf);

	// Write vertex coordinates
	mapper->MapDepthFrameToCameraSpace(width*height, buf, width*height, depth2xyz);
	float* fdest = (float*)dest;
	for (int i = 0; i < sz; i++) {
		*fdest++ = depth2xyz[i].X;
		*fdest++ = depth2xyz[i].Y;
		*fdest++ = depth2xyz[i].Z;
	}

	// Fill in depth2rgb map
	mapper->MapDepthFrameToColorSpace(width*height, buf, width*height, depth2rgb);
	if (depthframe) depthframe->Release();
}

void getRgbData(IMultiSourceFrame* frame, GLubyte* dest) {
	IColorFrame* colorframe;
	IColorFrameReference* frameref = NULL;
	frame->get_ColorFrameReference(&frameref);
	frameref->AcquireFrame(&colorframe);
	if (frameref) frameref->Release();

	if (!colorframe) return;

	// Get data from frame
	colorframe->CopyConvertedFrameDataToArray(colorwidth*colorheight*4, rgbimage, ColorImageFormat_Rgba);

	// Write color array for vertices
	float* fdest = (float*)dest;
	for (int i = 0; i < width*height; i++) {
		ColorSpacePoint p = depth2rgb[i];
		// Check if color pixel coordinates are in bounds
		if (p.X < 0 || p.Y < 0 || p.X > colorwidth || p.Y > colorheight) {
			*fdest++ = 0;
			*fdest++ = 0;
			*fdest++ = 0;
		}
		else {
			int idx = (int)p.X + colorwidth*(int)p.Y;
			*fdest++ = rgbimage[4*idx + 0]/255.;
			*fdest++ = rgbimage[4*idx + 1]/255.;
			*fdest++ = rgbimage[4*idx + 2]/255.;
		}
    }

	if (colorframe) colorframe->Release();
}

void getBodyData(IMultiSourceFrame* frame) {
	
	IBodyFrame* bodyframe;
	IBodyFrameReference* frameref = NULL;
	frame->get_BodyFrameReference(&frameref);
	frameref->AcquireFrame(&bodyframe);
	if (frameref) frameref->Release();

	if (!bodyframe) return;

	IBody* body[BODY_COUNT] = { 0 };
	IBody* bodycapture;
	bodyframe->GetAndRefreshBodyData(BODY_COUNT, body);
	for (int i = 0; i < BODY_COUNT; i++) {
		
		body[i]->get_IsTracked(&tracked);
		if (tracked) {
			body[i]->GetJoints(JointType_Count, joints);
			ccc++;
			//body[i].joints[JointType_SpineBase].Position;
			if (ccc <= c3) std::cout << ccc << " \n";
			if (ccc == c3) {
				
				std::cout << "Your body orientation is captured now, You can move! \n";
				bodycapture = body[i];
				bodycapture -> GetJoints(JointType_Count, jointscap);
			}
			
			break;
		}
	}

	if (bodyframe) bodyframe->Release();
}

void getKinectData() {
	IMultiSourceFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame))) {
		GLubyte* ptr;
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			getDepthData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, cboId);
		ptr = (GLubyte*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		if (ptr) {
			getRgbData(frame, ptr);
		}
		glUnmapBuffer(GL_ARRAY_BUFFER);
        getBodyData(frame);
	}
	if (frame) frame->Release();
}

void rotateCamera() {
	static double angle = 0.;
	static double radius = 3.;
	double x = radius*sin(angle);
	double z = radius/2 - radius*(cos(angle));
	if (ccc < c3)
	{
	 
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
	gluLookAt(	x,		0,		z,
				0,		0,		radius/2,
				0,		1,		0);
	angle += 0.001;
	} else {
		int pos;
		if(pt == 0) {
			VideoCapture cap (0);
			if (!cap.isOpened()) {
				std::cout << "There is some major error!";
			}
			Mat frame;
			namedWindow("frame", 1);
			cap >> frame;
			imshow ("frame", frame);
			waitKey(0);
			pt++;
		}
		std::cout << "Enter the view number (1-5)? \n";
		std::cin >> pos;
		
		switch(pos) {
			case 1:
				angle = 1.0472;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;

			case 2:
				angle = 2.0944;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;
			case 3:
				angle = 3.14159;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;
			case 4:
				angle = 4.18879;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;
			case 5:
				angle = 5.23599;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;
			default:
				angle = 0.;
				x = radius*sin(angle);
				z = radius/2 - radius*(cos(angle));
				glMatrixMode(GL_MODELVIEW);
    			glLoadIdentity(); 
				gluLookAt(	x,		0,		z,
							0,		0,		radius/2,
							0,		1,		0);
				break;
		}
	}
}

void drawKinectData() {

	
	getKinectData();
	rotateCamera();
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexPointer(3, GL_FLOAT, 0, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glColorPointer(3, GL_FLOAT, 0, NULL);
	
	glPointSize(1.f);
	glDrawArrays(GL_POINTS, 0, width*height);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);


	if (ccc < c3) {

		if (tracked) {
		// Draw some arms
			const CameraSpacePoint& a0 = joints[JointType_SpineBase].Position;
			const CameraSpacePoint& a1 = joints[JointType_SpineMid].Position;
			const CameraSpacePoint& a2 = joints[JointType_Neck].Position;;
			const CameraSpacePoint& a3 = joints[JointType_Head].Position;;
			const CameraSpacePoint& a4 = joints[JointType_ShoulderLeft].Position;;
			const CameraSpacePoint& a5 = joints[JointType_ElbowLeft].Position;;
			const CameraSpacePoint& a6 = joints[JointType_WristLeft].Position;;
			const CameraSpacePoint& a7 = joints[JointType_HandLeft].Position;;
			const CameraSpacePoint& a8 = joints[JointType_ShoulderRight].Position;;
			const CameraSpacePoint& a9 = joints[JointType_ElbowRight].Position;
			const CameraSpacePoint& a10 = joints[JointType_WristRight].Position;;
			const CameraSpacePoint& a11 = joints[JointType_HandRight].Position;;
			const CameraSpacePoint& a12 = joints[JointType_HipLeft].Position;;
			const CameraSpacePoint& a13 = joints[JointType_KneeLeft].Position;;
			const CameraSpacePoint& a14 = joints[JointType_AnkleLeft].Position;;
			const CameraSpacePoint& a15 = joints[JointType_FootLeft].Position;;
			const CameraSpacePoint& a16 = joints[JointType_HipRight].Position;;
			const CameraSpacePoint& a17 = joints[JointType_KneeRight].Position;;
			const CameraSpacePoint& a18 = joints[JointType_AnkleRight].Position;
			const CameraSpacePoint& a19 = joints[JointType_FootRight].Position;;
			const CameraSpacePoint& a20 = joints[JointType_SpineShoulder].Position;;
			const CameraSpacePoint& a21 = joints[JointType_HandTipLeft].Position;;
			const CameraSpacePoint& a22 = joints[JointType_ThumbLeft].Position;;
			const CameraSpacePoint& a23 = joints[JointType_HandTipRight].Position;;
			const CameraSpacePoint& a24 = joints[JointType_ThumbRight].Position;;
			
			glBegin(GL_LINES);
			glColor3f(0.f, 1.f, 0.f);
				
			glVertex3f(a19.X, a19.Y, a19.Z);
			glVertex3f(a18.X, a18.Y, a18.Z);

			glVertex3f(a18.X, a18.Y, a18.Z);
			glVertex3f(a17.X, a17.Y, a17.Z);

			glVertex3f(a17.X, a17.Y, a17.Z);
			glVertex3f(a16.X, a16.Y, a16.Z);

			glVertex3f(a16.X, a16.Y, a16.Z);
			glVertex3f(a0.X, a0.Y, a0.Z);

			glVertex3f(a15.X, a15.Y, a15.Z);
			glVertex3f(a14.X, a14.Y, a14.Z);

			glVertex3f(a14.X, a14.Y, a14.Z);
			glVertex3f(a13.X, a13.Y, a13.Z);

			glVertex3f(a13.X, a13.Y, a13.Z);
			glVertex3f(a12.X, a12.Y, a12.Z);

			glVertex3f(a12.X, a12.Y, a12.Z);
			glVertex3f(a0.X, a0.Y, a0.Z);

			glVertex3f(a0.X, a0.Y, a0.Z);
			glVertex3f(a1.X, a1.Y, a1.Z);

			glVertex3f(a1.X, a1.Y, a1.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a21.X, a21.Y, a21.Z);
			glVertex3f(a7.X, a7.Y, a7.Z);

			glVertex3f(a7.X, a7.Y, a7.Z);
			glVertex3f(a6.X, a6.Y, a6.Z);

			glVertex3f(a22.X, a22.Y, a22.Z);
			glVertex3f(a6.X, a6.Y, a6.Z);

			glVertex3f(a6.X, a6.Y, a6.Z);
			glVertex3f(a5.X, a5.Y, a5.Z);

			glVertex3f(a5.X, a5.Y, a5.Z);
			glVertex3f(a4.X, a4.Y, a4.Z);

			glVertex3f(a4.X, a4.Y, a4.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a23.X, a23.Y, a23.Z);
			glVertex3f(a11.X, a11.Y, a11.Z);

			glVertex3f(a11.X, a11.Y, a11.Z);
			glVertex3f(a10.X, a10.Y, a10.Z);
				
			glVertex3f(a24.X, a24.Y, a24.Z);
			glVertex3f(a10.X, a10.Y, a10.Z);

			glVertex3f(a10.X, a10.Y, a10.Z);
			glVertex3f(a9.X, a9.Y, a9.Z);

			glVertex3f(a9.X, a9.Y, a9.Z);
			glVertex3f(a8.X, a8.Y, a8.Z);

			glVertex3f(a8.X, a8.Y, a8.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a20.X, a20.Y, a20.Z);
			glVertex3f(a2.X, a2.Y, a2.Z);

			glVertex3f(a2.X, a2.Y, a2.Z);
			glVertex3f(a3.X, a3.Y, a3.Z);
			glEnd();		
		}
	} else {


			const CameraSpacePoint& a0 = jointscap[JointType_SpineBase].Position;
			const CameraSpacePoint& a1 = jointscap[JointType_SpineMid].Position;;
			const CameraSpacePoint& a2 = jointscap[JointType_Neck].Position;;
			const CameraSpacePoint& a3 = jointscap[JointType_Head].Position;;
			const CameraSpacePoint& a4 = jointscap[JointType_ShoulderLeft].Position;;
			const CameraSpacePoint& a5 = jointscap[JointType_ElbowLeft].Position;;
			const CameraSpacePoint& a6 = jointscap[JointType_WristLeft].Position;;
			const CameraSpacePoint& a7 = jointscap[JointType_HandLeft].Position;;
			const CameraSpacePoint& a8 = jointscap[JointType_ShoulderRight].Position;;
			const CameraSpacePoint& a9 = jointscap[JointType_ElbowRight].Position;
			const CameraSpacePoint& a10 = jointscap[JointType_WristRight].Position;;
			const CameraSpacePoint& a11 = jointscap[JointType_HandRight].Position;;
			const CameraSpacePoint& a12 = jointscap[JointType_HipLeft].Position;;
			const CameraSpacePoint& a13 = jointscap[JointType_KneeLeft].Position;;
			const CameraSpacePoint& a14 = jointscap[JointType_AnkleLeft].Position;;
			const CameraSpacePoint& a15 = jointscap[JointType_FootLeft].Position;;
			const CameraSpacePoint& a16 = jointscap[JointType_HipRight].Position;;
			const CameraSpacePoint& a17 = jointscap[JointType_KneeRight].Position;;
			const CameraSpacePoint& a18 = jointscap[JointType_AnkleRight].Position;
			const CameraSpacePoint& a19 = jointscap[JointType_FootRight].Position;;
			const CameraSpacePoint& a20 = jointscap[JointType_SpineShoulder].Position;;
			const CameraSpacePoint& a21 = jointscap[JointType_HandTipLeft].Position;;
			const CameraSpacePoint& a22 = jointscap[JointType_ThumbLeft].Position;;
			const CameraSpacePoint& a23 = jointscap[JointType_HandTipRight].Position;;
			const CameraSpacePoint& a24 = jointscap[JointType_ThumbRight].Position;


			// Saving the captured coordinates in the kinect order of joint numbers!
			std::ofstream myfile1("kinectdata.csv");
			myfile1 << std::fixed << std::setprecision(prec) << a0.X << "," << a0.Y << "," << a0.Z << "," << "\n" ;
			myfile1 << a1.X << "," << a1.Y << "," << a1.Z << "," << "\n" ;
			myfile1 << a2.X << "," << a2.Y << "," << a2.Z << "," << "\n" ;
			myfile1 << a3.X << "," << a3.Y << "," << a3.Z << "," << "\n" ;
			myfile1 << a4.X << "," << a4.Y << "," << a4.Z << "," << "\n" ;
			myfile1 << a5.X << "," << a5.Y << "," << a5.Z << "," << "\n" ;
			myfile1 << a6.X << "," << a6.Y << "," << a6.Z << "," << "\n" ;
			myfile1 << a7.X << "," << a7.Y << "," << a7.Z << "," << "\n" ;
			myfile1 << a8.X << "," << a8.Y << "," << a8.Z << "," << "\n" ;
			myfile1 << a9.X << "," << a9.Y << "," << a9.Z << "," << "\n" ;
			myfile1 << a10.X << "," << a10.Y << "," << a10.Z << "," << "\n" ;
			myfile1 << a11.X << "," << a11.Y << "," << a11.Z << "," << "\n" ;
			myfile1 << a12.X << "," << a12.Y << "," << a12.Z << "," << "\n" ;
			myfile1 << a13.X << "," << a13.Y << "," << a13.Z << "," << "\n" ;
			myfile1 << a14.X << "," << a14.Y << "," << a14.Z << "," << "\n" ;
			myfile1 << a15.X << "," << a15.Y << "," << a15.Z << "," << "\n" ;
			myfile1 << a16.X << "," << a16.Y << "," << a16.Z << "," << "\n" ;
			myfile1 << a17.X << "," << a17.Y << "," << a17.Z << "," << "\n" ;
			myfile1 << a18.X << "," << a18.Y << "," << a18.Z << "," << "\n" ;
			myfile1 << a19.X << "," << a19.Y << "," << a19.Z << "," << "\n" ;
			myfile1 << a20.X << "," << a20.Y << "," << a20.Z << "," << "\n" ;
			myfile1 << a21.X << "," << a21.Y << "," << a21.Z << "," << "\n" ;
			myfile1 << a22.X << "," << a22.Y << "," << a22.Z << "," << "\n" ;
			myfile1 << a23.X << "," << a23.Y << "," << a23.Z << "," << "\n" ;
			myfile1 << a24.X << "," << a24.Y << "," << a24.Z << "," << "\n" ;
			myfile1.close();
			glBegin(GL_LINES);

			glColor3f(0.f, 1.f, 0.f);
				
			glVertex3f(a19.X, a19.Y, a19.Z);
			glVertex3f(a18.X, a18.Y, a18.Z);

			glVertex3f(a18.X, a18.Y, a18.Z);
			glVertex3f(a17.X, a17.Y, a17.Z);

			glVertex3f(a17.X, a17.Y, a17.Z);
			glVertex3f(a16.X, a16.Y, a16.Z);

			glVertex3f(a16.X, a16.Y, a16.Z);
			glVertex3f(a0.X, a0.Y, a0.Z);

			glVertex3f(a15.X, a15.Y, a15.Z);
			glVertex3f(a14.X, a14.Y, a14.Z);

			glVertex3f(a14.X, a14.Y, a14.Z);
			glVertex3f(a13.X, a13.Y, a13.Z);

			glVertex3f(a13.X, a13.Y, a13.Z);
			glVertex3f(a12.X, a12.Y, a12.Z);

			glVertex3f(a12.X, a12.Y, a12.Z);
			glVertex3f(a0.X, a0.Y, a0.Z);

			glVertex3f(a0.X, a0.Y, a0.Z);
			glVertex3f(a1.X, a1.Y, a1.Z);

			glVertex3f(a1.X, a1.Y, a1.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a21.X, a21.Y, a21.Z);
			glVertex3f(a7.X, a7.Y, a7.Z);

			glVertex3f(a7.X, a7.Y, a7.Z);
			glVertex3f(a6.X, a6.Y, a6.Z);

			glVertex3f(a22.X, a22.Y, a22.Z);
			glVertex3f(a6.X, a6.Y, a6.Z);

			glVertex3f(a6.X, a6.Y, a6.Z);
			glVertex3f(a5.X, a5.Y, a5.Z);

			glVertex3f(a5.X, a5.Y, a5.Z);
			glVertex3f(a4.X, a4.Y, a4.Z);

			glVertex3f(a4.X, a4.Y, a4.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a23.X, a23.Y, a23.Z);
			glVertex3f(a11.X, a11.Y, a11.Z);

			glVertex3f(a11.X, a11.Y, a11.Z);
			glVertex3f(a10.X, a10.Y, a10.Z);
				
			glVertex3f(a24.X, a24.Y, a24.Z);
			glVertex3f(a10.X, a10.Y, a10.Z);

			glVertex3f(a10.X, a10.Y, a10.Z);
			glVertex3f(a9.X, a9.Y, a9.Z);

			glVertex3f(a9.X, a9.Y, a9.Z);
			glVertex3f(a8.X, a8.Y, a8.Z);

			glVertex3f(a8.X, a8.Y, a8.Z);
			glVertex3f(a20.X, a20.Y, a20.Z);

			glVertex3f(a20.X, a20.Y, a20.Z);
			glVertex3f(a2.X, a2.Y, a2.Z);

			glVertex3f(a2.X, a2.Y, a2.Z);
			glVertex3f(a3.X, a3.Y, a3.Z);
			glEnd();
	}
}

int main(int argc, char* argv[]) {
    if (!init(argc, argv)) return 1;
    if (!initKinect()) return 1;
	
	
		//if (waitKey(30) >= 0) break;
    // OpenGL setup
    glClearColor(0,0,0,0);
    glClearDepth(1.0f);

	// Set up array buffers
	const int dataSize = width*height * 3 * 4;
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cboId);
	glBindBuffer(GL_ARRAY_BUFFER, cboId);
	glBufferData(GL_ARRAY_BUFFER, dataSize, 0, GL_DYNAMIC_DRAW);
	
    // Camera setup
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(45, width /(GLdouble) height, 0.1, 1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	gluLookAt(0,0,0,0,0,1,0,1,0);

    // Main loop
    execute();
	
    return 0;
}




/*void rotateCamera() {                     //Original rotation version
	static double angle = 0.;
	static double radius = 3.;
	double x = radius*sin(angle);
	double z = radius/2 - radius*(cos(angle));
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity(); 
	gluLookAt(	x,		0,		z,
				0,		0,		radius/2,
				0,		1,		0);
	angle += 0.001;
}*/
