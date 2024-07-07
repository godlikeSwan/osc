#include <initguid.h>
#include <windows.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>

#include "font.hpp"

#include "device_enum.cpp"
#include "osc.cpp"

#define WIDTH 1280
#define HEIGHT 720

enum State {
    STATE_MENU,
    STATE_OSC
};

static State state;
static DeviceEnum *devEnum;
static UINT menuPosition;

static int refreshCount;

static Osc *osc;

static float scale;
static float freq;

char debugString[128];

void drawArrowRight(float x, float y, float size) {
    glBegin(GL_TRIANGLES);
        glVertex2f(x, y);
        glVertex2f(x, y + size);
        glVertex2f(x + size / sqrtf(3.0f), y + size / 2.0f);
    glEnd();
}

void changeMenuPosition(int d) {
    UINT oldPosition = 0;
    if (menuPosition == 0 && d < 0) {
        menuPosition = 0;
        return;
    }
    UINT count = devEnum->getCount();
    if (menuPosition + d + 1 > count) {
        menuPosition = count > 0 ? count - 1 : 0;
        return;
    }
    menuPosition += d;
}

void enterOsc() {
    state = STATE_OSC;
    osc = new Osc(devEnum->getDevice(menuPosition), 10.0f, WIDTH);
    osc->start();
    scale = 1.0f;
    freq = 1.0f;
}

void print(float x, float y, const char *text) {
    glRasterPos2f(x, y);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const char *c = text;
    while (*c) glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, font[*(c++) - 32]);
}

static void quit(GLFWwindow *window, int key, int scancode, int action, int mods) {
    switch (state) {
        case STATE_MENU: {
            if (action == GLFW_PRESS) {
                switch (key) {
                    case GLFW_KEY_UP:
                    case GLFW_KEY_W:
                    case GLFW_KEY_K:
                    changeMenuPosition(-1);
                    glClearColor(1.0f, .0f, .0f, .5f);
                    glClear(GL_COLOR_BUFFER_BIT);
                    break;
                    case GLFW_KEY_DOWN:
                    case GLFW_KEY_S:
                    case GLFW_KEY_J:
                    changeMenuPosition(+1);
                    break;
                    case GLFW_KEY_R:
                    devEnum->enumerate();
                    if (!strncmp(debugString, "refreshed", 9)) {
                        sprintf_s(debugString, 128, "refreshed x%i", refreshCount++);
                    } else {
                        refreshCount = 2;
                        sprintf_s(debugString, 128, "refreshed");
                    }
                    break;
                    case GLFW_KEY_ENTER:
                    enterOsc();
                    break;
                    case GLFW_KEY_Q:
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                    break;
                    default:
                    break;
                }
            }
            break;
        }
        case STATE_OSC: {
            if (action == GLFW_PRESS) {
                switch (key) {
                    case GLFW_KEY_UP:
                        scale *= 2;
                        break;
                    case GLFW_KEY_DOWN:
                        scale /= 2;
                        break;
                    case GLFW_KEY_RIGHT:
                        if (mods & GLFW_MOD_CONTROL) osc->freq *= 2;
                        else if (mods & GLFW_MOD_SHIFT) osc->freq += 1.0f;
                        else osc->freq += osc->freq / 10.0f;
                        if (osc->freq > (float)osc->sampleRate) osc->freq = (float)osc->sampleRate;
                        if (osc->freq < .01f) osc->freq = .01f;
                        // if (osc->freq * (float)osc->presicion > (float)osc->sampleRate) osc->freq = (float)osc->sampleRate / (float)osc->presicion;
                        break;
                    case GLFW_KEY_LEFT:
                        if (mods & GLFW_MOD_CONTROL) osc->freq /= 2;
                        else if (mods & GLFW_MOD_SHIFT) osc->freq -= 1.0f;
                        else osc->freq -= osc->freq / 10.0f;
                        break;
                    default:
                    break;
                }
            }
        }
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void printDevices(DeviceEnum *devEnum) {
    UINT count = devEnum->getCount();

    if (count == 0) {
        glColor3f(.9f, .0f, .0f);
        print(-.3f, .3f, "No devices found. Press 'R' to refresh");
        return;
    }
    if (menuPosition + 1 > count) menuPosition = count - 1;
    for (UINT i = 0; i < count; ++i) {
        CHAR str[128];
        devEnum->getDeviceName(i, str, 128);
        glColor3f(.5f, .0f, .5f);
        if (i == menuPosition) {
            glColor3f(.5f, .5f, .0f);
            drawArrowRight(-.35f, .6f - i * .08f, .023f);
        }
        print(-.3f, .6f - i * .08f, str);
    }
    return;
}

int main () {
    glfwInit();
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "osc", NULL, NULL);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, quit);
    debugString[0] = '!';
    debugString[1] = '\0';
    state = STATE_MENU;
    devEnum = new DeviceEnum();
    menuPosition = 0;

    osc = NULL;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        switch (state) {
            case STATE_MENU: {
                glClearColor(0, 0, 0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);

                glColor3f(.8f, .8f, .8f);
                print(-.3f, .8f, "Choose input device ('R' to refresh)");
                printDevices(devEnum);

                glColor3f(.5f, .5f, .5f);
                print(-1.0f, -1.0f, debugString);

                glFinish();
                break;
            }
            case STATE_OSC: {
                glClearColor(0, 0, 0, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);
                osc->processOnce();
                size_t i = osc->offset;
                int j = 0;
                glColor3f(.0f, .9f, .0f);
                glBegin(GL_LINE_STRIP);
                do {
                    glVertex2f((float)(j++) * 2 / (float)WIDTH - 1.0f, osc->frame[i] * scale > 1.0f ? 1.0f : osc->frame[i] * scale);
                    i += 1;
                    i %= WIDTH;
                } while (i != osc->offset);
                glEnd();

                glColor3f(.5f, .5f, .5f);
                print(-1.0f, -1.0f, debugString);
                glFinish();
            }
        }
    }

    if (osc != NULL) {
        osc->stop();
        delete osc;
    }
    if (devEnum != NULL) {
        delete devEnum;
        devEnum = NULL;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
