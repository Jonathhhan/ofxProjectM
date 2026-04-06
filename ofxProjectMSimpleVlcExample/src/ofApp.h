#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxProjectM.h"
#include "ofxVlc4.h"

#include <filesystem>

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();

	void keyPressed(int key);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void audioOut(ofSoundBuffer & buffer);

private:
	void setupProjectM();
	void setupPlayer();
	void setupSoundStream();
	void loadSeedMedia();
	bool loadMedia(const std::filesystem::path & path, bool playNow = true);
	std::string mediaStatusLabel() const;
	std::string projectMTextureModeLabel() const;
	void updateProjectMTextureBinding();
	void updateProjectMWindowSize();

	ofxProjectM projectM;
	ofxVlc4 player;
	ofxImGui::Gui gui;
	ofSoundStream soundStream;

	int sampleRate = 44100;
	int outputChannels = 2;
	int bufferSize = 512;
	float previewMargin = 20.0f;
	bool useVideoTextureForProjectM = true;
};
