
DEFINES =
MACHINENAME=
CXXFLAGS= -O2 -fomit-frame-pointer $(DEFINES)
override CXXFLAGS+= -Wall -fsigned-char -fno-exceptions -fno-rtti

PLATFORM= $(shell uname -s)
PLATFORM_PREFIX=sandbox

INCLUDES= -Ishared -Iengine -Ienet/include

STRIP=
ifeq (,$(findstring -g,$(CXXFLAGS)))
ifeq (,$(findstring -pg,$(CXXFLAGS)))
  STRIP=strip
endif
endif

MV=mv

ifneq (,$(findstring MINGW,$(PLATFORM)))
WINDRES= windres
CLIENT_INCLUDES= $(INCLUDES) -Iinclude
CLIENT_LIBS= -mwindows -Llib -lSDL -lSDL_image -lSDL_mixer -lzdll -lopengl32 -lenet -lws2_32 -lwinmm
else
CLIENT_INCLUDES= $(INCLUDES) -I/usr/X11R6/include `sdl-config --cflags`
CLIENT_LIBS= -Lenet/.libs -lenet -L/usr/X11R6/lib `sdl-config --libs` -lSDL_image -lSDL_mixer -lz -lGL -lX11
endif
ifeq ($(PLATFORM),Linux)
CLIENT_LIBS+= -lrt
ifneq (,$(findstring _64,$(shell uname -m)))
MACHINENAME=64
else
MACHINENAME=32
endif
endif

CLIENT_OBJS= \
	shared/crypto.o \
	shared/geom.o \
	shared/stream.o \
	shared/tools.o \
	shared/zip.o \
	engine/3dgui.o \
	engine/bih.o \
	engine/blend.o \
	engine/blob.o \
	engine/client.o	\
	engine/command.o \
	engine/console.o \
	engine/decal.o \
	engine/dynlight.o \
	engine/glare.o \
	engine/grass.o \
	engine/lightmap.o \
	engine/main.o \
	engine/material.o \
	engine/menus.o \
	engine/movie.o \
	engine/normal.o	\
	engine/octa.o \
	engine/octaedit.o \
	engine/octarender.o \
	engine/pvs.o \
	engine/physics.o \
	engine/rendergl.o \
	engine/rendermodel.o \
	engine/renderparticles.o \
	engine/rendersky.o \
	engine/rendertext.o \
	engine/renderva.o \
	engine/server.o	\
	engine/serverbrowser.o \
	engine/shader.o \
	engine/shadowmap.o \
	engine/sound.o \
	engine/texture.o \
	engine/ui.o \
	engine/water.o \
	engine/world.o \
	engine/worldio.o

FPSCLIENT_OBJS = \
	fpsgame/ai.o \
	fpsgame/client.o \
	fpsgame/entities.o \
	fpsgame/fps.o \
	fpsgame/monster.o \
	fpsgame/movable.o \
	fpsgame/render.o \
	fpsgame/scoreboard.o \
	fpsgame/server.o \
	fpsgame/waypoint.o

SSPCLIENT_OBJS = \
	sspgame/ssp.o \
	sspgame/sspent.o \
	sspgame/sspentities.o \
	sspgame/sspweapons.o \
	sspgame/sspmonster.o \
	sspgame/ssprender.o \
	sspgame/sspstubs.o

RPGCLIENT_OBJS = \
	rpggame/rpg.o \
	rpggame/rpgai.o \
	rpggame/rpgaction.o \
	rpggame/rpgcamera.o \
	rpggame/rpgchar.o \
	rpggame/rpgconfig.o \
	rpggame/rpgcontainer.o \
	rpggame/rpgeffect.o \
	rpggame/rpgentities.o \
	rpggame/rpghud.o \
	rpggame/rpggui.o \
	rpggame/rpgio.o \
	rpggame/rpgitem.o \
	rpggame/rpgobstacle.o \
	rpggame/rpgplatform.o \
	rpggame/rpgproj.o \
	rpggame/rpgscript.o \
	rpggame/rpgstats.o \
	rpggame/rpgstatus.o \
	rpggame/rpgstubs.o \
	rpggame/rpgrender.o \
	rpggame/rpgtest.o \
	rpggame/rpgtrigger.o \
	rpggame/waypoint.o

MOVIECLIENT_OBJS= \
	moviecube/action.o \
	moviecube/characterinfo.o \
	moviecube/client.o \
	moviecube/clientmode.o \
	moviecube/control.o \
	moviecube/entities.o \
	moviecube/mapentities.o \
	moviecube/movable.o \
	moviecube/moviecube.o \
	moviecube/render.o \
	moviecube/camera.o \
	moviecube/server.o


KRSCLIENT_OBJS = \
	krsgame/krs.o \
	krsgame/krsai.o \
	krsgame/krsents.o \
	krsgame/krshud.o \
	krsgame/krsrender.o \
	krsgame/krsstubs.o \
	krsgame/krsscript.o \
	krsgame/waypoint.o

PZLCLIENT_OBJS = \
	pzlgame/pzl.o \
	pzlgame/pzlents.o \
	pzlgame/pzlhud.o \
	pzlgame/pzlrender.o \
	pzlgame/pzlstubs.o \
	pzlgame/pzlscript.o

CLIENT_PCH = \
	shared/cube.h.gch \
	engine/engine.h.gch \
	fpsgame/game.h.gch \
	sspgame/sspgame.h.gch \
	rpggame/rpggame.h.gch \
	moviecube/game.h.gch \
	krsgame/krsgame.h.gch \
	pzlgame/pzlgame.h.gch

ifneq (,$(findstring MINGW,$(PLATFORM)))
SERVER_INCLUDES= -DSTANDALONE $(INCLUDES) -Iinclude
SERVER_LIBS= -mwindows -Llib -lzdll -lenet -lws2_32 -lwinmm
MASTER_LIBS= -Llib -lzdll -lenet -lws2_32 -lwinmm
else
SERVER_INCLUDES= -DSTANDALONE $(INCLUDES)
SERVER_LIBS= -Lenet/.libs -lenet -lz
MASTER_LIBS= $(SERVER_LIBS)
endif

SERVER_OBJS= \
	shared/crypto-standalone.o \
	shared/stream-standalone.o \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/server-standalone.o \
	engine/worldio-standalone.o

FPSSERVER_OBJS = \
	fpsgame/entities-standalone.o \
	fpsgame/server-standalone.o

MOVIESERVER_OBJS= \
	moviecube/server-standalone.o \
	moviecube/entities-standalone.o

MASTER_OBJS= \
	shared/crypto-standalone.o \
	shared/stream-standalone.o \
	shared/tools-standalone.o \
	engine/command-standalone.o \
	engine/master-standalone.o

LAUNCHER_LIBS= `wx-config --libs`
LAUNCHER_FLAGS= `wx-config --cxxflags`
LAUNCHER_OBJS= \
	launcher/pasl.o \
	launcher/paslapp.o

ifeq ($(PLATFORM),SunOS)
CLIENT_LIBS+= -lsocket -lnsl -lX11
SERVER_LIBS+= -lsocket -lnsl
endif

default: all

$(LAUNCHER_OBJS): CXXFLAGS += $(LAUNCHER_FLAGS)

pasl:   $(LAUNCHER_OBJS)
	$(CXX) $(CXXFLAGS) -o launch $(LAUNCHER_OBJS) $(LAUNCHER_LIBS)
	cp launch ../launcher
	strip ../launcher
	chmod +x ../launcher

all: client server

enet/Makefile:
	cd enet; ./configure --enable-shared=no --enable-static=yes

libenet: enet/Makefile
	$(MAKE)	-C enet/ all

clean-enet: enet/Makefile
	$(MAKE) -C enet/ clean

clean:
	-$(RM) $(SERVER_OBJS) $(FPSSERVER_OBJS) $(MOVIESERVER_OBJS) $(CLIENT_PCH) $(CLIENT_OBJS) $(SSPCLIENT_OBJS) $(RPGCLIENT_OBJS) $(FPSCLIENT_OBJS) $(MOVIECLIENT_OBJS) $(KRSCLIENT_OBJS) $(PZLCLIENT_OBJS) $(LAUNCHER_OBJS) $(MASTER_OBJS) $(PLATFORM_PREFIX)_server_* $(PLATFORM_PREFIX)_client_*

%.h.gch: %.h
	$(CXX) $(CXXFLAGS) -o $@.tmp $(subst .h.gch,.h,$@)
	$(MV) $@.tmp $@

%-standalone.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $(subst -standalone.o,.cpp,$@)

$(CLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES)

$(filter shared/%,$(CLIENT_OBJS)): $(filter shared/%,$(CLIENT_PCH))
$(filter engine/%,$(CLIENT_OBJS)): $(filter engine/%,$(CLIENT_PCH))

$(SSPCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Isspgame
$(SSPCLIENT_OBJS): $(filter sspgame/%,$(CLIENT_PCH))

$(FPSCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Ifpsgame
$(FPSCLIENT_OBJS): $(filter fpsgame/%,$(CLIENT_PCH))

$(RPGCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Irpggame
$(RPGCLIENT_OBJS): $(filter rpggame/%,$(CLIENT_PCH))

$(MOVIECLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Imoviecube
$(MOVIECLIENT_OBJS): $(filter moviecube/%,$(CLIENT_PCH))

$(KRSCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Ikrsgame
$(KRSCLIENT_OBJS): $(filter krsgame/%,$(CLIENT_PCH))

$(PZLCLIENT_OBJS): CXXFLAGS += $(CLIENT_INCLUDES) -Ipzlgame
$(PZLCLIENT_OBJS): $(filter pzlgame/%,$(CLIENT_PCH))

$(SERVER_OBJS): CXXFLAGS += $(SERVER_INCLUDES)
$(filter-out $(SERVER_OBJS),$(MASTER_OBJS)): CXXFLAGS += $(SERVER_INCLUDES)

$(FPSSERVER_OBJS): CXXFLAGS += -Ifpsgame $(SERVER_INCLUDES)
$(MOVIESERVER_OBJS): CXXFLAGS += -Imoviecube $(SERVER_INCLUDES)

client: client-fps client-krs client-movie client-rpg client-ssp client-pzl
server: server-fps server-movie


ifneq (,$(findstring MINGW,$(PLATFORM)))
client-fps:$(CLIENT_OBJS) $(FPSCLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/fps.rc -J rc -o windows/resources/fps.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_fps.exe windows/resources/fps.res $(CLIENT_OBJS) $(FPSCLIENT_OBJS) $(CLIENT_LIBS)

client-ssp:$(CLIENT_OBJS) $(SSPCLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/ssp.rc -J rc -o windows/resources/ssp.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_ssp.exe windows/resources/ssp.res $(CLIENT_OBJS) $(SSPCLIENT_OBJS) $(CLIENT_LIBS)

client-rpg:$(CLIENT_OBJS) $(RPGCLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/rpg.rc -J rc -o windows/resources/rpg.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_rpg.exe windows/resources/rpg.res $(CLIENT_OBJS) $(RPGCLIENT_OBJS) $(CLIENT_LIBS)

client-movie:$(CLIENT_OBJS) $(MOVIECLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/mc.rc -J rc -o windows/resources/mc.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_movie.exe windows/resources/mc.res $(CLIENT_OBJS) $(MOVIECLIENT_OBJS) $(CLIENT_LIBS)

client-krs:$(CLIENT_OBJS) $(KRSCLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/krs.rc -J rc -o windows/resources/krs.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_krs.exe windows/resources/krs.res $(CLIENT_OBJS) $(KRSCLIENT_OBJS) $(CLIENT_LIBS)

client-pzl:$(CLIENT_OBJS) $(PZLCLIENT_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/pzl.rc -J rc -o windows/resources/pzl.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_pzl.exe windows/resources/pzl.res $(CLIENT_OBJS) $(PZLCLIENT_OBJS) $(CLIENT_LIBS)

server-fps:$(SERVER_OBJS) $(FPSSERVEROBJS)
	$(WINDRES) -I windows/resources -i windows/resources/fps.rc -J rc -o windows/resources/fps.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_server_fps.exe windows/resources/fps.res $(SERVER_OBJS) $(FPSSERVER_OBJS) $(SERVER_LIBS)

server-movie: $(SERVER_OBJS) $(MOVIESERVER_OBJS)
	$(WINDRES) -I windows/resources -i windows/resources/mc.rc -J rc -o windows/resources/mc.res -O coff
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_server_movie.exe windows/resources/mc.res $(SERVER_OBJS) $(MOVIESERVER_OBJS) $(SERVER_LIBS)


master: $(MASTER_OBJS)
	$(CXX) $(CXXFLAGS) -o ../bin/sandbox_master.exe $(MASTER_OBJS) $(MASTER_LIBS)

install: all
else

client-fps:libenet $(CLIENT_OBJS) $(FPSCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_fps $(CLIENT_OBJS) $(FPSCLIENT_OBJS) $(CLIENT_LIBS)

client-ssp:libenet $(CLIENT_OBJS) $(SSPCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_ssp $(CLIENT_OBJS) $(SSPCLIENT_OBJS) $(CLIENT_LIBS)

client-rpg:libenet $(CLIENT_OBJS) $(RPGCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_rpg $(CLIENT_OBJS) $(RPGCLIENT_OBJS) $(CLIENT_LIBS)

client-movie:libenet $(CLIENT_OBJS) $(MOVIECLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_movie $(CLIENT_OBJS) $(MOVIECLIENT_OBJS) $(CLIENT_LIBS)

client-krs:libenet $(CLIENT_OBJS) $(KRSCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_krs $(CLIENT_OBJS) $(KRSCLIENT_OBJS) $(CLIENT_LIBS)

client-pzl:libenet $(CLIENT_OBJS) $(PZLCLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_client_$(MACHINENAME)_pzl $(CLIENT_OBJS) $(PZLCLIENT_OBJS) $(CLIENT_LIBS)

server-fps: libenet $(SERVER_OBJS) $(FPSSERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_server_$(MACHINENAME)_fps $(SERVER_OBJS) $(FPSSERVER_OBJS) $(SERVER_LIBS)

server-movie: libenet $(SERVER_OBJS) $(MOVIESERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_server_$(MACHINENAME)_movie $(SERVER_OBJS) $(MOVIESERVER_OBJS) $(SERVER_LIBS)

master: libenet $(MASTER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PLATFORM_PREFIX)_master $(MASTER_OBJS) $(MASTER_LIBS)


install: all
	cp $(PLATFORM_PREFIX)_* ../bin/
ifneq (,$(STRIP))
	strip ../bin/$(PLATFORM_PREFIX)_??????_$(MACHINENAME)*
endif
	chmod +x ../bin/$(PLATFORM_PREFIX)_??????_$(MACHINENAME)*
endif

shared/cube2font.o: shared/cube2font.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< `freetype-config --cflags`

cube2font: shared/cube2font.o
	$(CXX) $(CXXFLAGS) -o cube2font shared/cube2font.o `freetype-config --libs` -lz -lpng

depend:
	makedepend -Y -Ishared -Iengine -Ifpsgame -Isspgame $(subst .o,.cpp,$(CLIENT_OBJS))
	makedepend -a -o.h.gch -Y -Ishared -Iengine -Ifpsgame $(subst .h.gch,.h,$(CLIENT_PCH))
	makedepend -a -o-standalone.o -Y -Ishared -Iengine -Ifpsgame -Isspgame $(subst -standalone.o,.cpp,$(SERVER_OBJS))
	makedepend -a -o-standalone.o -Y -Ishared -Iengine -Ifpsgame $(subst -standalone.o,.cpp,$(filter-out $(SERVER_OBJS), $(MASTER_OBJS)))

engine/engine.h.gch: shared/cube.h.gch
fpsgame/game.h.gch: shared/cube.h.gch

# DO NOT DELETE

shared/crypto.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/crypto.o: shared/command.h shared/iengine.h shared/igame.h
shared/geom.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/geom.o: shared/command.h shared/iengine.h shared/igame.h
shared/stream.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/stream.o: shared/command.h shared/iengine.h shared/igame.h
shared/tools.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/tools.o: shared/command.h shared/iengine.h shared/igame.h
shared/zip.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
shared/zip.o: shared/command.h shared/iengine.h shared/igame.h
engine/3dgui.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/3dgui.o: shared/ents.h shared/command.h shared/iengine.h
engine/3dgui.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/3dgui.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/3dgui.o: engine/textedit.h
engine/bih.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/bih.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/bih.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/bih.o: engine/texture.h engine/model.h engine/varray.h
engine/blend.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blend.o: shared/ents.h shared/command.h shared/iengine.h
engine/blend.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/blend.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/blob.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/blob.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/blob.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/blob.o: engine/texture.h engine/model.h engine/varray.h
engine/client.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/client.o: shared/ents.h shared/command.h shared/iengine.h
engine/client.o: shared/igame.h engine/world.h engine/octa.h
engine/client.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/client.o: engine/model.h engine/varray.h
engine/command.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/command.o: shared/ents.h shared/command.h shared/iengine.h
engine/command.o: shared/igame.h engine/world.h engine/octa.h
engine/command.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/command.o: engine/model.h engine/varray.h
engine/console.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/console.o: shared/ents.h shared/command.h shared/iengine.h
engine/console.o: shared/igame.h engine/world.h engine/octa.h
engine/console.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/console.o: engine/model.h engine/varray.h
engine/decal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/decal.o: shared/ents.h shared/command.h shared/iengine.h
engine/decal.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/decal.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/dynlight.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/dynlight.o: shared/ents.h shared/command.h shared/iengine.h
engine/dynlight.o: shared/igame.h engine/world.h engine/octa.h
engine/dynlight.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/dynlight.o: engine/model.h engine/varray.h
engine/glare.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/glare.o: shared/ents.h shared/command.h shared/iengine.h
engine/glare.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/glare.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/glare.o: engine/rendertarget.h
engine/grass.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/grass.o: shared/ents.h shared/command.h shared/iengine.h
engine/grass.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/grass.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/lightmap.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/lightmap.o: shared/ents.h shared/command.h shared/iengine.h
engine/lightmap.o: shared/igame.h engine/world.h engine/octa.h
engine/lightmap.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/lightmap.o: engine/model.h engine/varray.h
engine/main.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/main.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/main.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/main.o: engine/texture.h engine/model.h engine/varray.h
engine/material.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/material.o: shared/ents.h shared/command.h shared/iengine.h
engine/material.o: shared/igame.h engine/world.h engine/octa.h
engine/material.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/material.o: engine/model.h engine/varray.h
engine/menus.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/menus.o: shared/ents.h shared/command.h shared/iengine.h
engine/menus.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/menus.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/movie.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/movie.o: shared/ents.h shared/command.h shared/iengine.h
engine/movie.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/movie.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/normal.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/normal.o: shared/ents.h shared/command.h shared/iengine.h
engine/normal.o: shared/igame.h engine/world.h engine/octa.h
engine/normal.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/normal.o: engine/model.h engine/varray.h
engine/octa.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octa.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/octa.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/octa.o: engine/texture.h engine/model.h engine/varray.h
engine/octaedit.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/octaedit.o: shared/ents.h shared/command.h shared/iengine.h
engine/octaedit.o: shared/igame.h engine/world.h engine/octa.h
engine/octaedit.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/octaedit.o: engine/model.h engine/varray.h
engine/octarender.o: engine/engine.h shared/cube.h shared/tools.h
engine/octarender.o: shared/geom.h shared/ents.h shared/command.h
engine/octarender.o: shared/iengine.h shared/igame.h engine/world.h
engine/octarender.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/octarender.o: engine/texture.h engine/model.h engine/varray.h
engine/physics.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/physics.o: shared/ents.h shared/command.h shared/iengine.h
engine/physics.o: shared/igame.h engine/world.h engine/octa.h
engine/physics.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/physics.o: engine/model.h engine/varray.h engine/mpr.h
engine/pvs.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/pvs.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
engine/pvs.o: engine/world.h engine/octa.h engine/lightmap.h engine/bih.h
engine/pvs.o: engine/texture.h engine/model.h engine/varray.h
engine/rendergl.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/rendergl.o: shared/ents.h shared/command.h shared/iengine.h
engine/rendergl.o: shared/igame.h engine/world.h engine/octa.h
engine/rendergl.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/rendergl.o: engine/model.h engine/varray.h
engine/rendermodel.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendermodel.o: shared/geom.h shared/ents.h shared/command.h
engine/rendermodel.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendermodel.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendermodel.o: engine/texture.h engine/model.h engine/varray.h
engine/rendermodel.o: engine/ragdoll.h engine/animmodel.h engine/vertmodel.h
engine/rendermodel.o: engine/skelmodel.h engine/md2.h engine/md3.h
engine/rendermodel.o: engine/md5.h engine/obj.h engine/smd.h
engine/renderparticles.o: engine/engine.h shared/cube.h shared/tools.h
engine/renderparticles.o: shared/geom.h shared/ents.h shared/command.h
engine/renderparticles.o: shared/iengine.h shared/igame.h engine/world.h
engine/renderparticles.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/renderparticles.o: engine/texture.h engine/model.h engine/varray.h
engine/renderparticles.o: engine/rendertarget.h engine/depthfx.h
engine/renderparticles.o: engine/explosion.h engine/lensflare.h
engine/renderparticles.o: engine/lightning.h
engine/rendersky.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendersky.o: shared/geom.h shared/ents.h shared/command.h
engine/rendersky.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendersky.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendersky.o: engine/texture.h engine/model.h engine/varray.h
engine/rendertext.o: engine/engine.h shared/cube.h shared/tools.h
engine/rendertext.o: shared/geom.h shared/ents.h shared/command.h
engine/rendertext.o: shared/iengine.h shared/igame.h engine/world.h
engine/rendertext.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/rendertext.o: engine/texture.h engine/model.h engine/varray.h
engine/renderva.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/renderva.o: shared/ents.h shared/command.h shared/iengine.h
engine/renderva.o: shared/igame.h engine/world.h engine/octa.h
engine/renderva.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/renderva.o: engine/model.h engine/varray.h
engine/server.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/server.o: shared/ents.h shared/command.h shared/iengine.h
engine/server.o: shared/igame.h engine/world.h engine/octa.h
engine/server.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/server.o: engine/model.h engine/varray.h
engine/serverbrowser.o: engine/engine.h shared/cube.h shared/tools.h
engine/serverbrowser.o: shared/geom.h shared/ents.h shared/command.h
engine/serverbrowser.o: shared/iengine.h shared/igame.h engine/world.h
engine/serverbrowser.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/serverbrowser.o: engine/texture.h engine/model.h engine/varray.h
engine/shader.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/shader.o: shared/ents.h shared/command.h shared/iengine.h
engine/shader.o: shared/igame.h engine/world.h engine/octa.h
engine/shader.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/shader.o: engine/model.h engine/varray.h
engine/shadowmap.o: engine/engine.h shared/cube.h shared/tools.h
engine/shadowmap.o: shared/geom.h shared/ents.h shared/command.h
engine/shadowmap.o: shared/iengine.h shared/igame.h engine/world.h
engine/shadowmap.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/shadowmap.o: engine/texture.h engine/model.h engine/varray.h
engine/shadowmap.o: engine/rendertarget.h
engine/sound.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/sound.o: shared/ents.h shared/command.h shared/iengine.h
engine/sound.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/sound.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/texture.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/texture.o: shared/ents.h shared/command.h shared/iengine.h
engine/texture.o: shared/igame.h engine/world.h engine/octa.h
engine/texture.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/texture.o: engine/model.h engine/varray.h engine/scale.h
engine/water.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/water.o: shared/ents.h shared/command.h shared/iengine.h
engine/water.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/water.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/world.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/world.o: shared/ents.h shared/command.h shared/iengine.h
engine/world.o: shared/igame.h engine/world.h engine/octa.h engine/lightmap.h
engine/world.o: engine/bih.h engine/texture.h engine/model.h engine/varray.h
engine/worldio.o: engine/engine.h shared/cube.h shared/tools.h shared/geom.h
engine/worldio.o: shared/ents.h shared/command.h shared/iengine.h
engine/worldio.o: shared/igame.h engine/world.h engine/octa.h
engine/worldio.o: engine/lightmap.h engine/bih.h engine/texture.h
engine/worldio.o: engine/model.h engine/varray.h

fpsgame/ai.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/ai.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
fpsgame/ai.o: fpsgame/ai.h
fpsgame/client.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/client.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/client.o: shared/igame.h fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/entities.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/entities.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/entities.o: shared/igame.h fpsgame/ai.h
fpsgame/fps.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/fps.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
fpsgame/fps.o: fpsgame/ai.h
fpsgame/monster.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/monster.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/monster.o: shared/igame.h fpsgame/ai.h
fpsgame/movable.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/movable.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/movable.o: shared/igame.h fpsgame/ai.h
fpsgame/render.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/render.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/render.o: shared/igame.h fpsgame/ai.h
fpsgame/scoreboard.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/scoreboard.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/scoreboard.o: shared/iengine.h shared/igame.h fpsgame/ai.h
fpsgame/server.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/server.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/server.o: shared/igame.h fpsgame/ai.h fpsgame/capture.h fpsgame/ctf.h
fpsgame/server.o: fpsgame/extinfo.h fpsgame/aiman.h
fpsgame/waypoint.o: fpsgame/game.h shared/cube.h shared/tools.h shared/geom.h
fpsgame/waypoint.o: shared/ents.h shared/command.h shared/iengine.h
fpsgame/waypoint.o: shared/igame.h fpsgame/ai.h

sspgame/ssp.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/ssp.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/ssp.o: sspgame/sspgame.h
sspgame/sspent.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/sspent.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/sspent.o: sspgame/sspgame.h
sspgame/ssprender.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/ssprender.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/ssprender.o: sspgame/sspgame.h
sspgame/sspmoster.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/sspmonster.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/sspmonster.o: sspgame/sspgame.h
sspgame/sspstubs.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/sspstubs.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/sspstubs.o: sspgame/sspgame.h
sspgame/sspweapons.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/sspweapons.o: shared/command.h shared/iengine.h shared/igame.h
sspgame/sspweapons.o: sspgame/sspgame.h

rpggame/rpg.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpg.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgai.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgai.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgaction.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgaction.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgcamera.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgcamera.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgchar.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgchar.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgconfig.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgconfig.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgcontainer.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgcontainer.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgeffect.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgeffect.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgentities.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgentities.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpggui.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpggui.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpghud.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpghud.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgio.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgio.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgitem.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgitem.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgobstacle.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgobstacle.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgplatform.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgplatform.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgproj.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgproj.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgscript.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgscript.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgstats.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgstats.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgstatus.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgstatus.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgstubs.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgstubs.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgrender.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgrender.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgtest.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgtest.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/rpgtrigger.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/rpgtrigger.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h
rpggame/waypoint.o : shared/cube.h shared/tools.h shared/geom.h shared/ents.h
rpggame/waypoint.o : shared/command.h shared/iengine.h shared/igame.h rpggame/rpggame.h

moviecube/client.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h moviecube/modeinfo.h moviecube/clientmode.h
moviecube/client.o: shared/ents.h shared/command.h shared/iengine.h
moviecube/client.o: shared/igame.h
moviecube/entities.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h moviecube/modeinfo.h moviecube/clientmode.h
moviecube/entities.o: shared/ents.h shared/command.h shared/iengine.h
moviecube/entities.o: shared/igame.h
moviecube/moviecube.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h moviecube/modeinfo.h moviecube/clientmode.h
moviecube/moviecube.o: shared/ents.h shared/command.h shared/iengine.h shared/igame.h
moviecube/movable.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h
moviecube/movable.o: shared/ents.h shared/command.h shared/iengine.h
moviecube/movable.o: shared/igame.h
moviecube/render.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h
moviecube/render.o: shared/ents.h shared/command.h shared/iengine.h
moviecube/render.o: shared/igame.h
moviecube/server.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h
moviecube/server.o: moviecube/game.h shared/cube.h shared/tools.h shared/geom.h moviecube/modeinfo.h moviecube/servmode.h
moviecube/server.o: shared/igame.h
moviecube/server.o: moviecube/extinfo.h moviecube/botmgr.h

krsgame/krs.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krs.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krs.o: krsgame/krsgame.h
krsgame/krsai.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsai.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsai.o: krsgame/krsgame.h
krsgame/krsents.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsents.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsents.o: krsgame/krsgame.h
krsgame/krshud.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krshud.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krshud.o: krsgame/krsgame.h
krsgame/krsrender.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsrender.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsrender.o: krsgame/krsgame.h
krsgame/krsstubs.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsstubs.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsstubs.o: krsgame/krsgame.h
krsgame/krsscript.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsscript.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsscript.o: krsgame/krsgame.h
krsgame/waypoint.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/waypoint.o: shared/command.h shared/iengine.h shared/igame.h
krsgame/waypoint.o: krsgame/krsgame.h

pzlgame/pzl.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzl.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzl.o: pzlgame/pzlgame.h
pzlgame/pzlents.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlents.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlents.o: pzlgame/pzlgame.h
pzlgame/pzlhud.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlhud.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlhud.o: pzlgame/pzlgame.h
pzlgame/pzlrender.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlrender.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlrender.o: pzlgame/pzlgame.h
pzlgame/pzlstubs.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlstubs.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlstubs.o: pzlgame/pzlgame.h
pzlgame/pzlscript.o: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlscript.o: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlscript.o: pzlgame/pzlgame.h

shared/cube.h.gch: shared/tools.h shared/geom.h shared/ents.h
shared/cube.h.gch: shared/command.h shared/iengine.h shared/igame.h
engine/engine.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
engine/engine.h.gch: shared/command.h shared/iengine.h shared/igame.h
engine/engine.h.gch: engine/world.h engine/octa.h engine/lightmap.h
engine/engine.h.gch: engine/bih.h engine/texture.h engine/model.h
engine/engine.h.gch: engine/varray.h
fpsgame/game.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
fpsgame/game.h.gch: shared/command.h shared/iengine.h shared/igame.h
fpsgame/game.h.gch: fpsgame/ai.h
sspgame/sspgame.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
sspgame/sspgame.h.gch: shared/command.h shared/iengine.h shared/igame.h
krsgame/krsgame.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
krsgame/krsgame.h.gch: shared/command.h shared/iengine.h shared/igame.h
pzlgame/pzlgame.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
pzlgame/pzlgame.h.gch: shared/command.h shared/iengine.h shared/igame.h
moviecube/game.h.gch: shared/cube.h shared/tools.h shared/geom.h shared/ents.h
moviecube/game.h.gch: shared/command.h shared/iengine.h shared/igame.h
moviecube/game.h.gch: moviecube/modeinfo.h moviecube/clientmode.h moviecube/servmode.h

shared/crypto-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/crypto-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/crypto-standalone.o: shared/igame.h
shared/stream-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/stream-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/stream-standalone.o: shared/igame.h
shared/tools-standalone.o: shared/cube.h shared/tools.h shared/geom.h
shared/tools-standalone.o: shared/ents.h shared/command.h shared/iengine.h
shared/tools-standalone.o: shared/igame.h
engine/command-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/command-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/command-standalone.o: shared/iengine.h shared/igame.h engine/world.h
engine/command-standalone.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/command-standalone.o: engine/texture.h engine/model.h engine/varray.h
engine/server-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/server-standalone.o: shared/iengine.h shared/igame.h engine/world.h
engine/server-standalone.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/server-standalone.o: engine/texture.h engine/model.h engine/varray.h
engine/worldio-standalone.o: engine/engine.h shared/cube.h shared/tools.h
engine/worldio-standalone.o: shared/geom.h shared/ents.h shared/command.h
engine/worldio-standalone.o: shared/iengine.h shared/igame.h engine/world.h
engine/worldio-standalone.o: engine/octa.h engine/lightmap.h engine/bih.h
engine/worldio-standalone.o: engine/texture.h engine/model.h engine/varray.h


fpsgame/entities-standalone.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/entities-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities-standalone.o: shared/iengine.h shared/igame.h fpsgame/ai.h
fpsgame/server-standalone.o: fpsgame/game.h shared/cube.h shared/tools.h
fpsgame/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/server-standalone.o: shared/iengine.h shared/igame.h fpsgame/ai.h
fpsgame/server-standalone.o: fpsgame/capture.h fpsgame/ctf.h
fpsgame/server-standalone.o: fpsgame/extinfo.h fpsgame/aiman.h

fpsgame/entities-standalone.o: moviecube/game.h shared/cube.h shared/tools.h
fpsgame/entities-standalone.o: shared/geom.h shared/ents.h shared/command.h
fpsgame/entities-standalone.o: shared/iengine.h shared/igame.h fpsgame/ai.h
moviecube/server-standalone.o: moviecube/game.h shared/cube.h shared/tools.h
moviecube/server-standalone.o: shared/geom.h shared/ents.h shared/command.h
moviecube/server-standalone.o: shared/iengine.h shared/igame.h
moviecube/server-standalone.o: moviecube/extinfo.h moviecube/botmgr.h
moviecube/server-standalone.o: moviecube/modeinfo.h moviecube/clientmode.h moviecube/servmode.h

engine/master-standalone.o: shared/cube.h shared/tools.h shared/geom.h
engine/master-standalone.o: shared/ents.h shared/command.h shared/iengine.h
engine/master-standalone.o: shared/igame.h
