#define _COSMO_SOURCE
#include <cosmo.h>
#include "libc/log/log.h"
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>



#define PROC(rt,name,params,args,return) \
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
		FLOGF(stderr, "loading opengl symbol: %s", #name); \
		dynapi_raw_##name = SDL_GL_GetProcAddress(#name); \
		if(!dynapi_raw_##name) exit(1); \
		if(IsWindows()) { \
			dynapi_##name = name##_WIN; \
		} \
		else { \
			dynapi_##name = dynapi_raw_##name; \
		} \
		FLOGF(stderr, "loaded opengl symbol: %s", #name); \
		return dynapi_##name args; \
	}

#include "o/gl/glcorearb.procs.h"
#undef PROC