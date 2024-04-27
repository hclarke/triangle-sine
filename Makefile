export SHELL = bash
export CC = $(SHELL) $(shell type -P cosmocc)
export CXX = bash cosmoc++
export AR = bash cosmoar
export ZIPOBJ = zipobj

CFLAGS = -O2

$(cd .)
SDL_URL_WIN = https://github.com/libsdl-org/SDL/releases/download/release-2.30.2/SDL2-2.30.2-win32-x64.zip
SDL_ZIP_WIN = o/third_party/SDL/SDL-win32-x64.zip
SDL_DLL = o/third_party/SDL/SDL2.dll
SDL_DLL_ZIPO = o/third_party/SDL/SDL2.dll.zip.o
$(SDL_DLL_ZIPO): ZIPOARGS += -N SDL2.dll

$(SDL_ZIP_WIN):
	@mkdir -p $(dir $@)
	curl -sL -o $@ $(SDL_URL_WIN)

$(SDL_DLL): $(SDL_ZIP_WIN)
	@mkdir -p $(dir $@)
	unzip -q -DD -o $< SDL2.dll -d $(dir $@)


GL_INCLUDES = -Ithird_party/OpenGL-Registry/api -Ithird_party/EGL-Registry/api
o/triangle-sine.com: $(SDL_DLL_ZIPO) o/extract.o o/sdl_dynapi.o o/gl_dynapi.o
o/triangle-sine.o: CFLAGS += -Ithird_party/SDL/include
o/triangle-sine.o: CFLAGS += $(GL_INCLUDES)
o/sdl_dynapi.o: CFLAGS += -Ithird_party/SDL/include

o/gl_dynapi.o: CFLAGS += -Ithird_party/SDL/include
o/gl_dynapi.o: CFLAGS += $(GL_INCLUDES)
o/gl_dynapi.o: o/gl/glcorearb.procs.h

o/gl/%.noinc.h: third_party/OpenGL-Registry/api/gl/%.h
	@mkdir -p $(dir $@)
	grep -v "^#include" $< > $@.tmp
	@mv $@.tmp $@

o/gl/%.exp.h: o/gl/%.noinc.h
	@mkdir -p $(dir $@)
	cosmocc -E -DGLAPI="" -DGL_GLEXT_PROTOTYPES $< > $@.tmp
	@mv $@.tmp $@

o/gl/%.procs.h: o/gl/%.exp.h gl2proc.py
	@mkdir -p $(dir $@)
	python gl2proc.py $< > $@.tmp
	@mv $@.tmp $@

o/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

o/%.com: o/%.o
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^

ls:
	ls $(dir $(CC))
	
o/%.zip.o: o/%
	@mkdir -p $(dir $@)/.aarch64
	$(ZIPOBJ) $(ZIPOARGS) -a x86_64 -o $@ -C 1 $<
	$(ZIPOBJ) $(ZIPOARGS) -a aarch64 -o $(dir $@)/.aarch64/$(notdir $@) -C 1 $<