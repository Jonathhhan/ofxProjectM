#pragma once

#include "ofMain.h"
#include "projectM-4/projectM.h"
#include "projectM-4/playlist.h"


class ofxProjectM {
public:
	~ofxProjectM();
	// init() creates the projectM engine, default render target, and preset playlist.
	void init();
	void setTexture(const ofTexture & texture);
	void clearTexture();
	void useInternalTextureOnly();
	void setWindowSize(int x, int y);
	void setMeshSize(int x, int y) const;
	void setPresetDuration(double duration) const;
	void reloadPresets();
	void update();
	void draw(int x, int y);
	void draw(int x, int y, int a, int b);
	const ofTexture & getTexture() const;
	void bind();
	void unbind();
	void audio(const float * buffer, int bufferSize, int channels) const;
	void previousPreset() const;
	void nextPreset() const;
	void randomPreset() const;
	int getPresetCount() const;
	int getPresetIndex() const;
	bool setPresetIndex(int index, bool hardCut = true) const;
	const std::string & getPresetName() const;
	int getMaxSamples() const;
	const std::string & getLastStatusMessage() const { return lastStatusMessage; }
	const std::string & getLastErrorMessage() const { return lastErrorMessage; }
	void clearLastMessages();
	static void textureLoadEvent(const char * textureName, projectm_texture_load_data * data, void * userData);
	static void presetSwitched(bool hardCut, unsigned int index, void* data);
	static void presetSwitchFailed(const char* presetFilename, const char* message, void* data);
private:
	static std::string formatPresetName(const char * presetPath);
	void setStatus(const std::string & message);
	void setError(const std::string & message);
	projectm_handle projectMHandle = nullptr;
	projectm_playlist_handle projectMPlaylistHandle = nullptr;
	ofTexture texture;
	ofFbo fbo;
	int windowWidth = 1024;
	int windowHeight = 1024;
	std::string presetName;
	std::string lastStatusMessage;
	std::string lastErrorMessage;
};
