#define _COSMO_SOURCE
#include <cosmo.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include "libc/log/log.h"
#include "libc/dlopen/dlfcn.h"
#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_vulkan.h"

extern int extract(const char *zip, const char *to);

void* try_dlopen(char* path) {
	FLOGF(stderr, "attempting dlopen: %s", path);
	void* lib = cosmo_dlopen(path, RTLD_LAZY | RTLD_LOCAL);
	if(lib) {
		FLOGF(stderr, "dlopened %s", path);
		return lib;
	}
	else {
		FLOGF(stderr, "failed to dlopen %s", path);
		return 0;
	}
}

void* sdl_lib;
void* dlopen_sdl() {
	void* sdl;

	char* sdl_path = getenv("SDL2_DYNAMIC_API");
	if(sdl_path) {
		if(strncmp("/zip/", sdl_path, 5) == 0) {
			if( (sdl = try_dlopen(sdl_path+5)) != 0 ) return sdl;
			extract(sdl_path, sdl_path+5);
			if( (sdl = try_dlopen(sdl_path+5)) != 0 ) return sdl;
		}
		else {
			if( (sdl = try_dlopen(sdl_path)) != 0 ) return sdl;
		}
	}
	if(IsWindows()) {
		if( (sdl = try_dlopen("SDL2.dll")) != 0 ) return sdl;
		//extract("/zip/SDL2.dll", "SDL2.dll");
		//if( (sdl = try_dlopen("SDL2.dll")) != 0 ) return sdl;
	}
	else { // this might work on mac
		if( (sdl = try_dlopen("libSDL2.so")) != 0 ) return sdl;
		if( (sdl = try_dlopen("libSDL2-2.0.0.so")) != 0 ) return sdl;
	}
	//TODO: other platforms
	return 0;
}

void init_sdl_dynapi() {
	if(sdl_lib) return;
	FLOGF(stderr, "loading SDL dynamic library");
	sdl_lib = dlopen_sdl();
	if(!sdl_lib) {
		FLOGF(stderr, "failed to load SDL dynamic library");
		exit(1);
	}
}


#define SDL_DYNAPI_PROC(rt,name,params,args,return) \
	rt name##_DEFAULT params; \
	void* dynapi_raw_##name; \
	rt (* dynapi_##name) params = name##_DEFAULT; \
	rt name##_WIN params { \
		rt (* __attribute__((__ms_abi__)) fn) params = dynapi_raw_##name; \
		return fn args; \
	} \
	inline rt name params { \
		return dynapi_##name args; \
	} \
	rt name##_DEFAULT params { \
		init_sdl_dynapi(); \
		FLOGF(stderr, "loading sdl symbol: %s", #name); \
		dynapi_raw_##name = cosmo_dlsym(sdl_lib, #name); \
		if(!dynapi_raw_##name) exit(1); \
		if(IsWindows()) { \
			dynapi_##name = name##_WIN; \
		} \
		else { \
			dynapi_##name = dynapi_raw_##name; \
		} \
		FLOGF(stderr, "loaded sdl symbol: %s", #name); \
		return dynapi_##name args; \
	}

#define SDL_OpenAudioDevice SDL_OpenAudioDevice_REAL
#define SDL_CloseAudioDevice SDL_CloseAudioDevice_REAL

#define SDL_DYNAPI_PROC_NO_VARARGS
#include "third_party/SDL/src/dynapi/SDL_dynapi_procs.h"

#undef SDL_OpenAudioDevice
#undef SDL_CloseAudioDevice

#undef SDL_DYNAMIC_API
#undef SDL_DYNAPI_PROC_NO_VARARGS

typedef struct {
	SDL_AudioCallback callback;
	void* userdata;
	SDL_AudioDeviceID deviceId;
} AudioWrapper;

AudioWrapper audio_wrappers[32];



void __attribute__((__ms_abi__)) SDL_AudioCallback_WIN(void *userdata, Uint8 *stream, int len) {
	AudioWrapper* wrapper = userdata;
	wrapper->callback(wrapper->userdata, stream, len);
}

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device,int iscapture,const SDL_AudioSpec *desired,SDL_AudioSpec *obtained,int allowed_changes) {

	if(desired->callback && IsWindows()) {	
		SDL_AudioSpec spec = *desired;
		AudioWrapper* wrapper = NULL;
		for(int i = 0; i < 32; ++i) {
			if(audio_wrappers[i].deviceId == 0) {
				wrapper = &audio_wrappers[i];
				break;
			}
		}
		if(!wrapper) {
			FLOGF(stderr, "too many audio devices");
			return 0;
		}
		wrapper->callback = desired->callback;
		wrapper->userdata = desired->userdata;
		spec.callback = (void*)SDL_AudioCallback_WIN;
		spec.userdata = wrapper;
		SDL_AudioDeviceID result = SDL_OpenAudioDevice_REAL(device, iscapture, &spec, obtained, allowed_changes);
		obtained->callback = desired->callback;
		obtained->userdata = desired->userdata;
		if(!result) return result;
		wrapper->deviceId = result;
		return result;
	}
	else {

		return SDL_OpenAudioDevice_REAL(device, iscapture, desired, obtained, allowed_changes);
	}
}

void SDL_CloseAudioDevice(SDL_AudioDeviceID device) {
	if(IsWindows()) {
		for(int i = 0; i < 32; ++i) {
			if(audio_wrappers[i].deviceId == device) {
				audio_wrappers[i].deviceId = 0;
			}
		}
	}
	SDL_CloseAudioDevice_REAL(device);
}