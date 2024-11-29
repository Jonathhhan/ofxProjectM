#pragma once

#include "ofMain.h"
#include "../libs/projectM/include/projectM-4/projectM.h"
#include "../libs/projectM/include/projectM-4/playlist.h"

class ofxProjectM {
public:
	~ofxProjectM();
	void load();
	void setWindowSize(int x, int y);
	void update();
	void draw(int x, int y);
	void draw(int x, int y, int a, int b);
	void bind();
	void unbind();
	void audio(float* buffer);
	void nextPreset();
	void randomPreset();
	char* getPresetName();
	int getMaxSamples();
private:
	projectm_handle projectMHandle;
	projectm_playlist_handle projectMPlaylistHandle;
	ofFbo fbo;
	int windowWidth, windowHeight;
};