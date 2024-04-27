#define _COSMO_SOURCE
#include <cosmo.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include "libc/log/log.h"
#define SDL_RENDER_VSYNC 1
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>


const GLchar* vertexSource =
    "attribute vec2 position;     \n"
    "attribute vec3 color;        \n"
	"varying vec3 vcolor;         \n"
    "void main()                  \n"
    "{                            \n"
	"	vcolor = color;           \n"
    "   gl_Position = vec4(position.xy, 0.0, 1.0);  \n"
    "}                            \n";
const GLchar* fragmentSource =
	"varying vec3 vcolor;         \n"
    "void main()                                  \n"
    "{                                            \n"
    "  gl_FragColor = vec4 (vcolor, 1.0 );\n"
    "}                                            \n";


typedef struct {
	float v;
	float vtarget;
	float oscv[3];
} audio_data;

void wave(void *userdata, Uint8 *stream, int len) {
	audio_data* ad = userdata;
	float* s = (void*)stream;
	for(int i = 0; i < len/4; ++i) {
		float f = 60.0/44100.0;
		s[i] = 0.0;
		float a = 1.0/1024;
		ad->v = ad->v * (1-a) + ad->vtarget * a;
		for(int j = 0; j < 3; ++j) {
			ad->oscv[j] += f;
			f *= 2.005;
			ad->oscv[j] -= floor(ad->oscv[j]);
			s[i] += sin(ad->oscv[j] * 3.14195927 * 2.0) * ad->v * 0.333;
		}
	}

}

int main(int argc, char** argv) {
	
	char* dir = strdup(argv[0]);
	dir = dirname(dir);
	if(!dir) goto EXIT;
	FLOGF(stderr, "dirname: %s", dir);
	chdir(dir);

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		FLOGF(stderr, "sdl init failed");
		goto EXIT;
	}

	SDL_version sdl_version;
	SDL_GetVersion(&sdl_version);
	FLOGF(stderr, "SDL Version: %d.%d.%d", sdl_version.major, sdl_version.minor, sdl_version.patch);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetSwapInterval(1);
	SDL_Window* window = SDL_CreateWindow("Δ~", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if(!window) goto EXIT;

	
    if(SDL_GL_LoadLibrary(NULL) < 0) {
        FLOGF(stderr, "failed to load opengl");
        exit(1);
    }


	SDL_Surface* window_surface = SDL_GL_CreateContext(window);
	if(!window_surface) {
		FLOGF(stderr, "Create Surface failed: %s", SDL_GetError());
		exit(1);
	}

	int major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	FLOGF(stderr, "opengl version: %d.%d", major, minor);

	FLOGF(stderr, "opengl version: %s", glGetString(GL_VERSION));

// Create Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLfloat vertices[] = {
		0.0f, 0.5f, 1.0, 0.0, 0.0,
		0.5f, -0.5f, 0.0, 1.0, 0.0,
		-0.5f, -0.5f, 0.0, 0.0, 1.0
	};

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create and compile the vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile the fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Link the vertex and fragment shader into a shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), 0);

	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(2*sizeof(float)));

	SDL_AudioSpec spec, nspec = {0};
	spec.freq = 44100;
	spec.format = AUDIO_F32;
	spec.channels = 1;
	spec.samples = 4096;
	spec.callback = wave;
	audio_data* ad = malloc(sizeof(audio_data));
	ad->v = 0.0;
	ad->vtarget = 1.0;
	for(int i = 0; i < 3; ++i) ad->oscv[i] = 0.0;
	spec.userdata = ad;
	int audioDevice = SDL_OpenAudioDevice(NULL, 0, &spec, &nspec, 0);
	if(audioDevice <= 0) {
		FLOGF(stderr, "failed to open audio device: %s", SDL_GetError());
		exit(1);
	}
	FLOGF(stderr, "audio spec: freq %d, ch %d, samp %d, fmt %d", nspec.freq, nspec.channels, nspec.samples, nspec.format);
	SDL_PauseAudioDevice(audioDevice, 0);
	while(true) {
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT) goto EXIT;
		}
		
        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 3);
        SDL_GL_SwapWindow(window);
	}
	EXIT:
	if(audioDevice) {
		ad->vtarget = 0.0;
		SDL_Delay(150);
		SDL_CloseAudioDevice(audioDevice);
	}
	FLOGF(stderr, "bye!");
}