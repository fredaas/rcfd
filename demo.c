#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glu.h>

#define IX(i, j) ((i) + (N + 2) * (j))

/* External definitions from solver.c */
extern void dens_step(int N, float *x, float *x0, float *u, float *v,
    float diff, float dt);
extern void vel_step(int N, float *u, float *v, float *u0, float *v0,
    float visc, float dt);

static int N;
static int lever;

static int win_x = 800, win_y = 800;

static float dt, diff, visc;
static float force, source;
static float *u, *v, *u_prev, *v_prev;
static float *dens, *dens_prev;

enum {
    MOUSE_LEFT,
    MOUSE_RIGHT
};

static int mouse_down[2];

static int omx, omy, mx, my;

GLFWwindow* window;


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

    u         = (float *)malloc(size * sizeof(float));
    v         = (float *)malloc(size * sizeof(float));
    u_prev    = (float *)malloc(size * sizeof(float));
    v_prev    = (float *)malloc(size * sizeof(float));
    dens      = (float *)malloc(size * sizeof(float));
    dens_prev = (float *)malloc(size * sizeof(float));

    if (!u || !v || !u_prev || !v_prev || !dens || !dens_prev)
    {
        printf("[ERROR] Failed to allocate data\n");
        return 0;
    }

    return 1;
}


/*******************************************************************************
 *
 * OpenGL drawing
 *
 ******************************************************************************/


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

    for (i = 0; i < N * N; i++)
        dens[i] *= 0.98;

    glEnd();
}


/*******************************************************************************
 *
 * Mouse movements
 *
 ******************************************************************************/


static void get_mouse_movement(float *d, float *u, float *v)
{
    int i, j, size = (N + 2) * (N + 2);

    for (i = 0; i < size; i++)
    {
        u[i] = v[i] = d[i] = 0.0f;
    }

    if (!mouse_down[MOUSE_LEFT] && !mouse_down[MOUSE_RIGHT])
        return;

    i = (int)((mx / (float)win_x) * N + 1);
    j = (int)(((win_y - my) / (float)win_y) * N + 1);

    if (i < 1 || i > N || j < 1 || j > N)
        return;

    #define abs(v) ((v) < 0 ? (v) * (-1) : (v))

    if (mouse_down[MOUSE_LEFT])
    {
        // u[IX(i, j)] = force * (mx - omx);
        v[IX(i, j)] = force * abs(omy - my);
    }

    if (mouse_down[MOUSE_RIGHT])
    {
        d[IX(i, j)] = source;
    }

    omx = mx;
    omy = my;

    return;
}


/*******************************************************************************
 *
 * Callbacks
 *
 ******************************************************************************/


static void key_callback(GLFWwindow* window, int key, int scancode, int action,
    int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_Q:
            free_data();
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_C:
            clear_data();
            break;
        case GLFW_KEY_V:
            lever = !lever;
            break;
        }

    }
}

static void cursor_position_callback(GLFWwindow* window, double xpos,
    double ypos)
{
    mx = xpos;
    my = ypos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        mouse_down[MOUSE_LEFT]  = (button == GLFW_MOUSE_BUTTON_LEFT);
        mouse_down[MOUSE_RIGHT] = (button == GLFW_MOUSE_BUTTON_RIGHT);
    }
    if (action == GLFW_RELEASE)
    {
        mouse_down[MOUSE_LEFT]  = 0;
        mouse_down[MOUSE_RIGHT] = 0;
    }
}


/*******************************************************************************
 *
 * Utils
 *
 ******************************************************************************/


void center_window(GLFWwindow *window)
{
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();

    if (!monitor)
        return;

    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    if (!mode)
        return;

    int monitor_x, monitor_y;
    glfwGetMonitorPos(monitor, &monitor_x, &monitor_y);

    int window_width, window_height;
    glfwGetWindowSize(window, &window_width, &window_height);

    glfwSetWindowPos(
        window,
        monitor_x + (mode->width - window_width) / 2,
        monitor_y + (mode->height - window_height) / 2
    );
}

void initialize(void)
{
    if (!glfwInit())
    {
        printf("[ERROR] Failed to initialize glfw\n");
        exit(1);
    }

    /* Configure window */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    window = glfwCreateWindow(win_x, win_y, "GLFW Window", NULL, NULL);
    if (!window)
    {
        printf("[ERROR] Failed to initialize window\n");
        exit(1);
    }
    center_window(window);
    glfwSwapInterval(1);
    glfwMakeContextCurrent(window);

    /* Set callbacks */
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    lever = 0;

    if (!allocate_data())
        exit(1);

    clear_data();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);
    glClear(GL_COLOR_BUFFER_BIT);
}

int main(int argc, char **argv)
{
    argc--;

    if (argc == 0)
    {
        N = 64;          // Grid divisor
        dt = 0.09f;      // Time spacing
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
    else if (argc == 6)
    {
        N = atoi(argv[1]);
        dt = atof(argv[2]);
        diff = atof(argv[3]);
        visc = atof(argv[4]);
        force = atof(argv[5]);
        source = atof(argv[6]);
    }
    else
    {
        printf("[ERROR] Expected 6 arguments\n");
    }

    initialize();

    printf(
        "Keys:\n"
        "    'v'           Toggle density/velocity fields\n"
        "    'c'           Clear fields\n"
        "    'right-mouse' Add densitites\n"
        "    'left-mouse'  Add velocities\n"
        "    'q'           Quit\n"
    );

    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &win_x, &win_y);
        glViewport(0, 0, win_x, win_y);

        get_mouse_movement(dens_prev, u_prev, v_prev);
        vel_step(N, u, v, u_prev, v_prev, visc, dt);
        dens_step(N, dens, dens_prev, u, v, diff, dt);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, 1.0, 0.0, 1.0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (lever)
            draw_velocity();
        else
            draw_density();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
