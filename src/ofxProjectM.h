#pragma once

#include "ofMain.h"
#include "../libs/projectM/include/projectM-4/projectM.h"
#include "../libs/projectM/include/projectM-4/playlist.h"

class ofxProjectM {
public:
	~ofxProjectM();
	void load();
	void setWindowSize(int x, int y);
	void setMeshSize(int x, int y) const;
	void setPresetDuration(double duration) const;
	void update();
	void draw(int x, int y);
	void draw(int x, int y, int a, int b);
	ofTexture getTexture();
	void bind();
	void unbind();
	void audio(float * buffer, int bufferSize, int channels) const;
	void nextPreset() const;
	void randomPreset() const;
	std::string getPresetName();
	int getMaxSamples();
	static void presetSwitched(bool hardCut, unsigned int index, void* data);
	static void presetSwitchFailed(const char* presetFilename, const char* message, void* data);
private:
	projectm_handle projectMHandle;
	projectm_playlist_handle projectMPlaylistHandle;
	ofFbo fbo;
	int windowWidth, windowHeight;
	std::string presetName;
};
