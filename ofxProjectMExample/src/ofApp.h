#pragma once

#include "ofMain.h"
#include "ofxProjectM.h"

class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		void audioOut(ofSoundBuffer & buffer);
		
		
		ofSoundStream soundStream;

		float pan;
		int sampleRate;
		bool bNoise;
		float volume;
		int bufferSize;
		int outputChannels;

		vector <float> lAudio;
		vector <float> rAudio;
		
		//------------------- for the simple sine wave synthesis
		float targetFrequency;
		float phase;
		float phaseAdder;
		float phaseAdderTarget;

		ofxProjectM projectM;
		ofBoxPrimitive box;
		ofEasyCam cam;
};
