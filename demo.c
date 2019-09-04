#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

#define IX(i, j) ((i) + (N + 2) * (j))

/* external definitions from solver.c */

extern void dens_step(int N, float *x, float *x0, float *u, float *v,
                      float diff, float dt);
extern void vel_step(int N, float *u, float *v, float *u0, float *v0,
                     float visc, float dt);

/* global variables */

static int N;
static float dt, diff, visc;
static float force, source;
static int dvel;

static float *u, *v, *u_prev, *v_prev;
static float *dens, *dens_prev;

static int win_id;
static int win_x, win_y;
static int mouse_down[3];
static int omx, omy, mx, my;


/*******************************************************************************
 *
 * Allocate/free data
 *
 ******************************************************************************/


static void free_data(void)
{
    if (u)
        free(u);
    if (v)
        free(v);
    if (u_prev)
        free(u_prev);
    if (v_prev)
        free(v_prev);
    if (dens)
        free(dens);
    if (dens_prev)
        free(dens_prev);
}

static void clear_data(void)
{
    int i, size = (N + 2) * (N + 2);

    for (i = 0; i < size; i++)
    {
        u[i] = v[i] = u_prev[i] = v_prev[i] = dens[i] = dens_prev[i] = 0.0f;
    }
}

static int allocate_data(void)
{
    int size = (N + 2) * (N + 2);

    u = (float *)malloc(size * sizeof(float));
    v = (float *)malloc(size * sizeof(float));
    u_prev = (float *)malloc(size * sizeof(float));
    v_prev = (float *)malloc(size * sizeof(float));
    dens = (float *)malloc(size * sizeof(float));
    dens_prev = (float *)malloc(size * sizeof(float));

    if (!u || !v || !u_prev || !v_prev || !dens || !dens_prev)
    {
        fprintf(stderr, "cannot allocate data\n");
        return (0);
    }

    return (1);
}


/*******************************************************************************
 *
 * OpenGL drawing
 *
 ******************************************************************************/


static void pre_display(void)
{
    glViewport(0, 0, win_x, win_y);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void post_display(void) { glutSwapBuffers(); }

static void draw_velocity(void)
{
    int i, j;
    float x, y, h;

    h = 1.0f / N;

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);

    glBegin(GL_LINES);

    for (i = 1; i <= N; i++)
    {
        x = (i - 0.5f) * h;
        for (j = 1; j <= N; j++)
        {
            y = (j - 0.5f) * h;

            glVertex2f(x, y);
            glVertex2f(x + u[IX(i, j)], y + v[IX(i, j)]);
        }
    }

    glEnd();
}

static void draw_density(void)
{
    int i, j;
    float x, y, h, d00, d01, d10, d11;

    h = 1.0f / N;

    glBegin(GL_QUADS);

    for (i = 0; i <= N; i++)
    {
        x = (i - 0.5f) * h;
        for (j = 0; j <= N; j++)
        {
            y = (j - 0.5f) * h;

            d00 = dens[IX(i, j)];
            d01 = dens[IX(i, j + 1)];
            d10 = dens[IX(i + 1, j)];
            d11 = dens[IX(i + 1, j + 1)];

            glColor3f(d00, d00, d00);
            glVertex2f(x, y);
            glColor3f(d10, d10, d10);
            glVertex2f(x + h, y);
            glColor3f(d11, d11, d11);
            glVertex2f(x + h, y + h);
            glColor3f(d01, d01, d01);
            glVertex2f(x, y + h);
        }
    }

    glEnd();
}


/*******************************************************************************
 *
 * Mouse movements
 *
 ******************************************************************************/


static void get_from_UI(float *d, float *u, float *v)
{
    int i, j, size = (N + 2) * (N + 2);

    for (i = 0; i < size; i++)
    {
        u[i] = v[i] = d[i] = 0.0f;
    }

    if (!mouse_down[0] && !mouse_down[2])
        return;

    i = (int)((mx / (float)win_x) * N + 1);
    j = (int)(((win_y - my) / (float)win_y) * N + 1);

    if (i < 1 || i > N || j < 1 || j > N)
        return;

    if (mouse_down[0])
    {
        u[IX(i, j)] = force * (mx - omx);
        v[IX(i, j)] = force * (omy - my);
    }

    if (mouse_down[2])
    {
        d[IX(i, j)] = source;
    }

    omx = mx;
    omy = my;

    return;
}


/*******************************************************************************
 *
 * Window callback routines
 *
 ******************************************************************************/


static void key_func(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 'c':
        clear_data();
        break;
    case 'q':
        free_data();
        exit(0);
        break;
    case 'v':
        dvel = !dvel;
        break;
    }
}

static void mouse_func(int button, int state, int x, int y)
{
    omx = mx = x;
    omx = my = y;

    mouse_down[button] = state == GLUT_DOWN;
}

static void motion_func(int x, int y)
{
    mx = x;
    my = y;
}

static void idle_func(void)
{
    get_from_UI(dens_prev, u_prev, v_prev);
    vel_step(N, u, v, u_prev, v_prev, visc, dt);
    dens_step(N, dens, dens_prev, u, v, diff, dt);

    glutSetWindow(win_id);
    glutPostRedisplay();
}

static void display_func(void)
{
    pre_display();

   if (dvel)
       draw_velocity();
   else
        draw_density();

    post_display();
}


/*******************************************************************************
 *
 * Window management
 *
 ******************************************************************************/


static void open_glut_window(void)
{
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    glutInitWindowPosition(0, 0);
    glutInitWindowSize(win_x, win_y);
    win_id = glutCreateWindow("RCFD");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

    pre_display();

    glutKeyboardFunc(key_func);
    glutMouseFunc(mouse_func);
    glutMotionFunc(motion_func);
    glutIdleFunc(idle_func);
    glutDisplayFunc(display_func);
}


int main(int argc, char **argv)
{
    glutInit(&argc, argv);

    if (argc == 1)
    {
        N = 64;          // Grid divisor
        dt = 0.05f;      // Time spacing
        diff = 0.0001f;  // Diffusion rate
        visc = 0.0f;     // Viscosity
        force = 5.0f;    // Velocity force
        source = 50.0f;  // Amount of density released at source
        printf(
            "Parameters:\n"
            "    N:      %d\n"
            "    dt:     %g\n"
            "    diff:   %g\n"
            "    visc:   %g\n"
            "    force:  %g\n"
            "    source: %g\n",
            N, dt, diff, visc, force, source
        );
    }
    else
    {
        N = atoi(argv[1]);
        dt = atof(argv[2]);
        diff = atof(argv[3]);
        visc = atof(argv[4]);
        force = atof(argv[5]);
        source = atof(argv[6]);
    }
    
    printf(
        "Keys:\n"
        "    'v'           Toggle density/velocity fields\n"
        "    'c'           Clear fields\n"
        "    'right-mouse' Add densitites\n"
        "    'left-mouse'  Add velocities\n"
        "    'q'           Quit\n"
    );

    dvel = 0;

    if (!allocate_data())
        exit(1);
    clear_data();

    win_x = 800;
    win_y = 600;
    open_glut_window();

    glutMainLoop();

    exit(0);
}
