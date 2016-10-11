#include <glut.h>
#include <gl.h>
#include <GLAUX.H>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <windows.h>
#include <iostream>
#include <time.h>
#include <string.h>
#include <sstream>
using namespace std;

//Vector
class Vec {    
public:   
	double x, y, z;    // position, also color (r,g,b)
	
	Vec operator+(const Vec &b) const{ return Vec(x+b.x,y+b.y,z+b.z); }
	Vec operator-(const Vec &b) const{ return Vec(x-b.x,y-b.y,z-b.z); }
	Vec operator*(double b)const { return Vec(x*b,y*b,z*b); }
	// cross product
	Vec operator%(const Vec&b) const{return Vec(y*b.z-z*b.y,z*b.x-x*b.z,x*b.y-y*b.x);}

	Vec(double x_=0, double y_=0, double z_=0){ x=x_; y=y_; z=z_; }
	Vec mult(const Vec &b) { return Vec(x*b.x,y*b.y,z*b.z); }
	Vec& norm(){ return *this = *this * (1/sqrt(x*x+y*y+z*z)); }
	
	//dot product
	double dot(const Vec &b) const { return x*b.x+y*b.y+z*b.z; }
	double len(){ return sqrt(x*x+y*y+z*z); }	
	void print()
	{
		cout << x << ' ' << y << ' ' << z << endl;
	}
};

//Parameters

//CONSTs
const float Pi = 3.141592654;
const float ROTATE_ANGLE = 12.0 / 180.0 * Pi;  
float goal_size = 0.8;
float fovy = 45.0f;
const int FRAME_WIDTH  = 650;
const int FRAME_HEIGHT = 650;
int window_width = FRAME_WIDTH, window_height = FRAME_HEIGHT;

//View: camera and retina model
Vec eye(3,4.5,4.5); //view point
Vec focus(0,0,0);	//view destination

Vec view_mid = eye + (focus - eye).norm(); //the middle of the rectina
Vec view_proj = Vec(focus.x-eye.x, 0, focus.z-eye.z); //focus->eye, projection on the floor

//coordinate system on Rectine
Vec camera_vy = (view_proj + Vec(0, pow(view_proj.len(),2) / eye.y ,0)).norm();  
Vec camera_vx = ((focus - eye) % camera_vy) .norm(); 
float view_size = tan(22.5 / 180 * Pi);
float view_vec_x = (focus.x - eye.x),  view_vec_y = (focus.y - eye.y), view_vec_z = (focus.z - eye.z);  
float theta0 = atan(sqrt(pow(eye.x - focus.x,2) + pow(eye.z - focus.z,2)) / eye.y);

//parameters for objects 
float table_sx = 2.2f, table_sy = 2.4f, table_sz = 4.2f;
float floor_size = 20.0f, floor_x = 0, floor_y = - table_sy, floor_z = 0.0f;
float edge_size = 0.1;

float Puck_x = 0, Puck_y = 0, Puck_z = 0, Puck_radius = 0.2 / 2, Puck_height = 0.08; //radius 0.06  height 0.05
float Puck_vx = 1, Puck_vy = 0, Puck_vz = 0.6;

float Mallet_radius =  Puck_radius / 2 * 3;
float Mallet_ai_x = 0, Mallet_ai_y = 0, Mallet_ai_z = -table_sz / 5;
float Mallet_ai_R = -0.8, Mallet_ai_G = -0.6, Mallet_ai_B = 1.0;

float Mallet_pl_x = 0, Mallet_pl_y = 0, Mallet_pl_z = table_sz / 4;
float Mallet_pl_R = 0.2, Mallet_pl_G = 0.1, Mallet_pl_B = 0.3;

//Movement
float time_interval = 15; // millis
float move_step = 0.05;
bool end_flag = false;
bool slip_flag = false;


//Scores
int score_ai = 0;
int score_pl = 0;
bool score_new = true;
int winner = 0;

//Textures
GLuint  texture[1];
AUX_RGBImageRec *LoadBMP(CHAR *Filename) 
{
	FILE *File=NULL;
	if (!Filename)          // Check for input
		return NULL;         
		
	File=fopen(Filename,"r");       // Try to find the file
	if (File)
	{
		fclose(File);
		return auxDIBImageLoadA(Filename);    // Load related BMP file and return 
	}
	return NULL;          
}

// Related to text objects
void selectFont(int size, int charset, const char* face) {  
    HFONT hFont = CreateFontA(size, 0, 0, 0, FW_MEDIUM, 0, 0, 0,  
        charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,  
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, face);  
    HFONT hOldFont = (HFONT)SelectObject(wglGetCurrentDC(), hFont);  
    DeleteObject(hOldFont);  
}  
void drawString(const char* str) //屏幕显示字体  
{  
    static int isFirstCall = 1;  
    static GLuint lists;  
	int MAX_CHAR = 128;
    if (isFirstCall) {  
        isFirstCall = 0;  
        // 申请MAX_CHAR个连续的显示列表编号  
        lists = glGenLists(MAX_CHAR);  
        // 把每个字符的绘制命令都装到对应的显示列表中  
        wglUseFontBitmaps(wglGetCurrentDC(), 0, MAX_CHAR, lists);  
    }  
    // 调用每个字符对应的显示列表，绘制每个字符  
    for (; *str != '\0'; ++str) {  
        glCallList(lists + *str);  
    }  
}  

//Initialization for OpenGL
void init(void) 
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup

	GLfloat light_position[] = { -1, 1, 1, 0};
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);							
	AUX_RGBImageRec *TextureImage[1];
	memset(TextureImage,0,sizeof(void *)*1);   // 将指针设为 NULL
	if (TextureImage[0]=LoadBMP("floor.bmp"))
	{
		glGenTextures(1, &texture[0]);     // Generate texture
		glBindTexture(GL_TEXTURE_2D, texture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
		
		//Parameters setup
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
						GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
						GL_LINEAR);
	}
	glEnable(GL_TEXTURE_2D);
}

//Draw floor with textures
void drawFloor(float pos_x, float pos_y, float pos_z) 
{
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_SPECULAR);

	glLoadIdentity();
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTranslatef(pos_x, pos_y, pos_z);
	glNormal3f(0.0,1.0,0.0);
	glBegin(GL_QUADS);						
		glColor3f(1.0f,1.0f,1.0f);	
		glNormal3f(0.0,1.0,0.0);
		glTexCoord2f(0.0, 0.0); 	    glVertex3f(-floor_size, 0, +floor_size);			
		glTexCoord2f(0.0, 40.0);		glVertex3f(-floor_size, 0, -floor_size);			
		glTexCoord2f(40.0, 40.0);		glVertex3f(+floor_size, 0, -floor_size);			
		glTexCoord2f(40.0, 0.0);		glVertex3f(+floor_size, 0, +floor_size);			
	glEnd();

	glDisable(GL_COLOR_MATERIAL);
	glBindTexture(GL_TEXTURE_2D,NULL);
}

//Draw table base and surface
void drawTable(float sx, float sy, float sz)
{
	//draw table base
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_SPECULAR);
	glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);

	glLoadIdentity();
	glTranslatef(0, floor_y + sy / 2, 0);				
	glColor3f(0.1f,0.4f,0.2f);
	glScaled(sx,sy,sz);
	glutSolidCube(1);

	//draw green table surface
	glLoadIdentity();
	glTranslatef(0, floor_y + sy + 0.001f, 0);				// Move Right 1.5 Units And Into The Screen 6.0
	glBegin(GL_QUADS);									// Draw A Quad
		glColor3f(-0.5f,1.0f,0.0f);
		glNormal3f(0.0,1.0,0.0);
		glVertex3f(-sx / 2, 0, sz / 2);			// Top Right Of The Quad (Top)
		glVertex3f(-sx / 2, 0, -sz / 2);			// Top Left Of The Quad (Top)
		glVertex3f(+sx / 2, 0, -sz / 2);			// Bottom Left Of The Quad (Top)
		glVertex3f(+sx / 2, 0, +sz / 2);			// Bottom Right Of The Quad (Top)
	glEnd();

	
	// BACK EDGES * 2
	// FRONT EDGES * 2
	// LEFT EDGE
	// RIGHT EDGE
	Vec pos[6] = {
		Vec(goal_size/2 + (sx-goal_size)/4, floor_y + sy + edge_size / 2, -sz / 2 + edge_size / 2),
		Vec(-(goal_size/2 + (sx-goal_size)/4), floor_y + sy + edge_size / 2, -sz / 2 + edge_size / 2),
		Vec(goal_size/2 + (sx-goal_size)/4, floor_y + sy + edge_size / 2, sz / 2 - edge_size / 2),
		Vec(-(goal_size/2 + (sx-goal_size)/4), floor_y + sy + edge_size / 2, sz / 2 - edge_size / 2), 
		Vec(-sx / 2 + edge_size / 2, floor_y + sy + edge_size / 2, 0), 
		Vec(+sx / 2 - edge_size / 2, floor_y + sy + edge_size / 2, 0) 
	};
	Vec scale[6] = { 
		Vec((sx-goal_size)/2,edge_size,edge_size),
		Vec((sx-goal_size)/2,edge_size,edge_size),
		Vec((sx-goal_size)/2,edge_size,edge_size),
		Vec((sx-goal_size)/2,edge_size,edge_size),
		Vec(edge_size,edge_size,sz),
		Vec(edge_size,edge_size,sz)
	};

	//draw back edge
	for(int obj = 0; obj < 6; obj++)
	{
		glLoadIdentity(); 
		if(obj < 4)
			glTranslatef( pos[obj].x, pos[obj].y, pos[obj].z);				// Move Right 1.5 Units And Into The Screen 6.0
		else
			glTranslatef( pos[obj].x, pos[obj].y, pos[obj].z - 0.001);	
		glColor3f(1.0f,1.0f,1.0f);
		glScaled(scale[obj].x,scale[obj].y,scale[obj].z);
		glutSolidCube(1);
	}


}


void drawPuck(float x, float y, float z)
{
	Vec Puck_color(0,-0.9,0.9);
	//draw surface
	GLUquadricObj *objCylinder = gluNewQuadric();
	glLoadIdentity();
	glTranslatef(x, 0, z);
	glColor3f(-0.4,0,0);
	glRotated(-90, 1,0,0);
	gluCylinder(objCylinder, Puck_radius, Puck_radius, Puck_height, 32, 5);
	
	glLoadIdentity();
	glTranslatef(x, Puck_height,  z);
	glColor3f(Puck_color.x,Puck_color.y,Puck_color.z);
	
	//draw circle
	glBegin(GL_POLYGON);
	float Pi = 3.141592654;
	int n = 200;
    for(int i=0;i<n;i++)
        glVertex3f(Puck_radius*cos(2*Pi/n*i), 0,Puck_radius*sin(2*Pi/n*i));
    glEnd();
}

// the same as Puck
void drawMallet(float x, float y, float z, int is_AI)
{
	float Mallet_R,Mallet_G,Mallet_B;
	if(is_AI)
	{
		Mallet_R = Mallet_ai_R;
		Mallet_G = Mallet_ai_G;
		Mallet_B = Mallet_ai_B;
	}
	else
	{
		Mallet_R = Mallet_pl_R;
		Mallet_G = Mallet_pl_G;
		Mallet_B = Mallet_pl_B;
	}

	//draw surface
	GLUquadricObj *objCylinder = gluNewQuadric();
	glLoadIdentity();
	glTranslatef(x, 0, z);
	glColor3f(Mallet_R - 0.3,Mallet_G - 0.3, Mallet_B - 0.3);
	glRotated(-90, 1,0,0);
	
	gluCylinder(objCylinder, Mallet_radius, Mallet_radius, Puck_height, 32, 5);
	
	glLoadIdentity();
	glTranslatef(x, Puck_height,  z);
	glColor3f(Mallet_R,Mallet_G,Mallet_B);
	
	//Draw circle
	glBegin(GL_POLYGON);
	int n = 200;
    for(int i=0;i<n;i++)
        glVertex3f(Mallet_radius*cos(2*Pi/n*i), 0,Mallet_radius*sin(2*Pi/n*i));
    glEnd();
}

void drawResult()
{
	Vec text_place = view_mid + camera_vx * -0.3 * view_size;
	if(winner == 1)
	{
		glColor3f(Mallet_pl_R,Mallet_pl_G,Mallet_pl_B); 
		glLoadIdentity();
		selectFont(100, ANSI_CHARSET, "Comic Sans MS"); 
		glRasterPos3f(text_place.x, text_place.y, text_place.z);
		string info = "YOU WIN!";
		drawString(info.c_str());
	}
	else if(winner == -1)
	{
		glColor3f(Mallet_ai_R,Mallet_ai_G,Mallet_ai_B);
		glLoadIdentity();
		selectFont(100, ANSI_CHARSET, "Comic Sans MS"); 
		glRasterPos3f(text_place.x, text_place.y, text_place.z);
		string info = "AI WIN!";
		drawString(info.c_str());
	}

}

// draw score on the top-left of the screen
void drawScore()
{
	glColor3f(1,0.0f,0.0f);//设置当前的绘图颜色为红色      
	glLoadIdentity();
	selectFont(80, ANSI_CHARSET, "Comic Sans MS");  //字体可以在系统文件里寻找  
	
	Vec pivot = view_mid + camera_vx * -0.8 * view_size  + camera_vy * 0.8 * view_size;
	glRasterPos3f(pivot.x, pivot.y, pivot.z);
	
	string pl_s, ai_s;
	stringstream s;
	s << score_pl;
	s >> pl_s;
	s.clear();
	s << score_ai;
	s >> ai_s;
	s.clear();

	string info = "Player: " + pl_s + "   AI: " + ai_s;
	drawString(info.c_str());
}

// Main Display Function
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	
	drawResult();
	drawScore();
	drawFloor(floor_x, floor_y, floor_z);
	drawTable(table_sx, table_sy, table_sz);
	drawPuck(Puck_x, Puck_y, Puck_z);	
	drawMallet(Mallet_ai_x, Mallet_ai_y, Mallet_ai_z,1);
	drawMallet(Mallet_pl_x, Mallet_pl_y, Mallet_pl_z,0);

	glutSwapBuffers();
}


void reshape (int width, int height)
{
	if (height==0)										// Prevent A Divide By Zero By
		height=1;										// Making Height Equal One
	glViewport(0,0,width,height);						// Reset The Current Viewport
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix
	gluPerspective(fovy,(GLfloat)width/(GLfloat)height,0.1f,100.0f);
	gluLookAt(eye.x, eye.y, eye.z, focus.x, focus.y, focus.z, 0, 1, 0);
	
	// refresh parameters for views
	window_width = width;
	window_height = height;
	view_mid = eye + (focus - eye).norm();
	view_proj = Vec(focus.x-eye.x, 0, focus.z-eye.z);
	camera_vy = (view_proj + Vec(0, pow(view_proj.len(),2) / eye.y ,0)).norm();
	camera_vx = ((focus - eye) % camera_vy) .norm();

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
		case 13: //Enter Key
			{
				Puck_x = 0;
				Puck_z = 0;
				Puck_vx = rand() % 200 - 100;
				Puck_vz = rand() % 200 - 100;
				winner = 0;
				slip_flag = false;
				end_flag = false;
				score_new = true;
				break;
			}
		default:
			break;
	}
}

void control_view(int key, int x, int y)
{
	switch(key)
	{
		case GLUT_KEY_UP:
			{
				if (eye.y < 9)	eye.y += 0.1;
				reshape(window_width, window_height);
				break;
			}
		case GLUT_KEY_DOWN:
			{
				if (eye.y > 1.5)	eye.y -= 0.1;
				reshape(window_width, window_height);
				break;
			}
		case GLUT_KEY_LEFT:
			{
				double r = view_proj.len();
				//view_proj.print();
				Vec vertical(view_proj.z, 0, -view_proj.x);
				eye = eye + view_proj.norm() * r * (1.0 - cos(ROTATE_ANGLE)) + vertical.norm() * r * sin(ROTATE_ANGLE);
				reshape(window_width, window_height);
				break;
			}
		case GLUT_KEY_RIGHT:
			{
				double r = view_proj.len();
				Vec vertical(-view_proj.z, 0, view_proj.x);
				eye = eye + view_proj.norm() * r * (1.0 - cos(ROTATE_ANGLE)) + vertical.norm() * r * sin(ROTATE_ANGLE);
				reshape(window_width, window_height);
				break;
			}
		default:
			break;
	}
}

void movement(int value)
{
	//collision detection
	if(fabs(Puck_x) > (table_sx/2 - edge_size - Puck_radius)) // LEFT and RIGHT walls
		Puck_vx = - Puck_vx;
	if(fabs(Puck_z) > (table_sz/2 - edge_size - Puck_radius)) // UP and DOWN walls
	{
		if(fabs(Puck_x) > goal_size / 2 && !slip_flag)
			Puck_vz = - Puck_vz;
		else
		{
			slip_flag = true; // the game is over, but will slip for a while
			if(Puck_z > 0)
			{
				winner = -1;
				if(score_new)
					score_ai += 1;	
			}
			else
			{
				winner = 1;
				if(score_new)
					score_pl += 1;
			}
			score_new = false;
		}
		if (fabs(Puck_z) > (table_sz/2 - edge_size - Puck_radius) + 0.2)
			end_flag = true;
	}

	//within Mallet_AI's range
	if( pow(Puck_x - Mallet_ai_x,2) + pow(Puck_z - Mallet_ai_z,2) < pow(Mallet_radius + Puck_radius,2) )
	{
		Puck_vx = Puck_x - Mallet_ai_x;
		Puck_vz = Puck_z - Mallet_ai_z;
	}
	//within Mallet_PL's range
	if( pow(Puck_x - Mallet_pl_x,2) + pow(Puck_z - Mallet_pl_z,2) < pow(Mallet_radius + Puck_radius,2) )
	{
		Puck_vx = Puck_x - Mallet_pl_x;
		Puck_vz = Puck_z - Mallet_pl_z;
	}
	if(sqrt(Puck_vx*Puck_vx + Puck_vz*Puck_vz) < 1e-12)
	{
		Puck_vx = rand();
		Puck_vz = rand();
	}

	Vec Puck(Puck_x, 0, Puck_z);
	
	Vec dir(Puck_vx, 0, Puck_vz);
	if(!end_flag)
	{//The game is not over
		if( Puck_x > (table_sx/2 - edge_size - Puck_radius) ) // should move within the table's range
			Puck_x = (table_sx/2 - edge_size - Puck_radius) + 0.001;
		else if( Puck_x < -(table_sx/2 - edge_size - Puck_radius))
			Puck_x = -(table_sx/2 - edge_size - Puck_radius) + 0.001;
		
		if(Puck_z > (table_sz/2 - edge_size - Puck_radius) && !slip_flag ) // UP and DOWN walls
			Puck_z = (table_sz/2 - edge_size - Puck_radius);
		else if (Puck_z < - (table_sz/2 - edge_size - Puck_radius) && !slip_flag)
			Puck_z = -(table_sz/2 - edge_size - Puck_radius);
		Puck_x += Puck_vx / sqrt(Puck_vx*Puck_vx + Puck_vz*Puck_vz) * move_step;
		Puck_z += Puck_vz / sqrt(Puck_vx*Puck_vx + Puck_vz*Puck_vz) * move_step;
	}

	Vec pre = Puck + dir.norm() * (Puck.z - Mallet_ai_z);

	Mallet_ai_x += 0.07 * (Puck_x - Mallet_ai_x);

	glutPostRedisplay();
	glutTimerFunc(time_interval, movement, 1);
}

// calculating Player's Mallet position according to the Mouse motion
void mouse_motion(int x, int y)
{
	float rad_x = (2.0 * x / window_width - 1);
	float rad_y = -(2.0 * y / window_height - 1);
	Vec pivot = view_mid + camera_vx * (double)rad_x * view_size  + camera_vy * (double)rad_y * view_size;
	Vec dir = Vec(pivot.x - eye.x, 0, pivot.z - eye.z);
	double ext_len = eye.y * dir.len() / (eye.y - pivot.y); 
	Vec nn = Vec(eye.x, 0 , eye.z) + dir.norm() * ext_len;

	if(fabs(nn.x) < table_sx/2 - edge_size - Mallet_radius)
		Mallet_pl_x = nn.x;
	if(nn.z < table_sz/2 - edge_size - Mallet_radius && nn.z > 0)
		Mallet_pl_z = nn.z;

	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize (FRAME_WIDTH, FRAME_HEIGHT); 
	glutInitWindowPosition (100, 100);
	glutCreateWindow (argv[0]);
	init();										//Initializing OpenGL parameters
	glutDisplayFunc(display);					//Registered to display function
	glutTimerFunc(time_interval, movement, 1);	// Start the timer
	glutPassiveMotionFunc(mouse_motion);		//Respond the Mouse motion
	glutReshapeFunc(reshape);					//When the windows size is changed
	glutKeyboardFunc(keyboard);					//Responded to keyboard event
	glutSpecialFunc(control_view);				//Responded to keyboard event
	glutIdleFunc( display );					
	glutMainLoop();          
	return 0;
}