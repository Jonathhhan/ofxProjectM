#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofBackground(34, 34, 34);
	ofSetWindowTitle("ofxProjectMExample");
	ofDisableArbTex();
	cam.setPosition(0, 0, 200);
	projectM.setWindowSize(1024, 1024);
	projectM.init();
	gui.setup(nullptr, true, ImGuiConfigFlags_None, true);
	ImGui::GetIO().IniFilename = "imgui_projectm.ini";
	std::cout << "Max samples: " << projectM.getMaxSamples() << std::endl;

	bufferSize = 512;
	outputChannels = 2;
	sampleRate = 44100;
	phase = 0;
	phaseAdder = 0.0f;
	phaseAdderTarget = 0.0f;
	volume = 0.1f;
	bNoise = false;

	lAudio.assign(bufferSize, 0.0);
	rAudio.assign(bufferSize, 0.0);
	
	soundStream.printDeviceList();

	ofSoundStreamSettings settings;

	// if you want to set the device id to be different than the default:
	//
	// auto devices = soundStream.getDeviceList();
	// settings.setOutDevice(devices[1]);

	// you can also get devices for an specific api:
	//
	//	auto devices = soundStream.getDeviceList(ofSoundDevice::Api::PULSE);
	//	settings.setOutDevice(devices[0]);

	// or get the default device for an specific api:
	//
	// settings.api = ofSoundDevice::Api::PULSE;

	// or by name:
	//
	//	auto devices = soundStream.getMatchingDevices("default");
	//	if(!devices.empty()){
	//		settings.setOutDevice(devices[0]);
	//	}

#ifdef TARGET_LINUX
	// Latest linux versions default to the HDMI output
	// this usually fixes that. Also check the list of available
	// devices if sound doesn't work
	auto devices = soundStream.getMatchingDevices("default");
	if(!devices.empty()){
		settings.setOutDevice(devices[0]);
	}
#endif

	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = outputChannels;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);

	// on OSX: if you want to use ofSoundPlayer together with ofSoundStream you need to synchronize buffersizes.
	// use ofFmodSetBuffersize(bufferSize) to set the buffersize in fmodx prior to loading a file.
}

//--------------------------------------------------------------
void ofApp::update(){
	projectM.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofNoFill();
	
	// draw the left channel:
	ofPushStyle();
		ofPushMatrix();
		ofTranslate(32, 150, 0);
			
		ofSetColor(225);
		ofDrawBitmapString("Left Channel", 4, 18);
		
		ofSetLineWidth(1);	
		ofDrawRectangle(0, 0, 900, 200);

		ofSetColor(245, 58, 135);
		ofSetLineWidth(3);
					
			ofBeginShape();
			for (unsigned int i = 0; i < lAudio.size(); i++){
				float x =  ofMap(i, 0, lAudio.size(), 0, 900, true);
				ofVertex(x, 100 -lAudio[i]*180.0f);
			}
			ofEndShape(false);
			
		ofPopMatrix();
	ofPopStyle();

	// draw the right channel:
	ofPushStyle();
		ofPushMatrix();
		ofTranslate(32, 350, 0);
			
		ofSetColor(225);
		ofDrawBitmapString("Right Channel", 4, 18);
		
		ofSetLineWidth(1);	
		ofDrawRectangle(0, 0, 900, 200);

		ofSetColor(245, 58, 135);
		ofSetLineWidth(3);
					
			ofBeginShape();
			for (unsigned int i = 0; i < rAudio.size(); i++){
				float x =  ofMap(i, 0, rAudio.size(), 0, 900, true);
				ofVertex(x, 100 -rAudio[i]*180.0f);
			}
			ofEndShape(false);
			
		ofPopMatrix();
	ofPopStyle();
	
	ofFill();
	cam.begin();
	projectM.bind();
	ofEnableDepthTest();
	box.draw();
	ofDisableDepthTest();
	projectM.unbind();
	cam.end();

	gui.begin();
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(340.0f, 260.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("ProjectM Controls")) {
		ImGui::TextWrapped("Standalone projectM example with generated audio driving the visuals.");
		ImGui::Separator();
		ImGui::Text("Preset: %s", projectM.getPresetName().c_str());
		ImGui::Text("Audio stream: %s", audioRunning ? "running" : "stopped");
		ImGui::Text("Buffer: %d  Sample rate: %d", bufferSize, sampleRate);

		if (ImGui::Button("Previous Preset")) {
			projectM.previousPreset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Next Preset")) {
			projectM.nextPreset();
		}
		if (ImGui::Button("Random Preset")) {
			projectM.randomPreset();
		}

		if (audioRunning) {
			if (ImGui::Button("Stop Audio")) {
				soundStream.stop();
				audioRunning = false;
			}
		} else {
			if (ImGui::Button("Start Audio")) {
				soundStream.start();
				audioRunning = true;
			}
		}

		ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
		ImGui::SliderFloat("Pan", &pan, 0.0f, 1.0f);
		ImGui::Checkbox("Noise", &bNoise);
		ImGui::Text("Target frequency: %.1f Hz", targetFrequency);
		ImGui::TextWrapped("Keyboard fallback: S start audio, E stop audio, +/- volume, any other key randomizes the preset.");
	}
	ImGui::End();
	gui.end();
}

//--------------------------------------------------------------
void ofApp::exit() {
	soundStream.close();
	gui.exit();
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
	if (key == '-' || key == '_' ){
		volume -= 0.05;
		volume = MAX(volume, 0);
	} else if (key == '+' || key == '=' ){
		volume += 0.05;
		volume = MIN(volume, 1);
	}
	
	if( key == 's' ){
		soundStream.start();
		audioRunning = true;
	}
	
	if( key == 'e' ){
		soundStream.stop();
		audioRunning = false;
	}
	projectM.randomPreset();
}

//--------------------------------------------------------------
void ofApp::keyReleased  (int key){
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	int width = ofGetWidth();
	pan = (float)x / (float)width;
	float height = (float)ofGetHeight();
	float heightPct = ((height-y) / height);
	targetFrequency = 2000.0f * heightPct;
	phaseAdderTarget = (targetFrequency / (float) sampleRate) * TWO_PI;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	int width = ofGetWidth();
	pan = (float)x / (float)width;
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	bNoise = true;
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	bNoise = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer) {
	//pan = 0.5f;
	float leftScale = 1 - pan;
	float rightScale = pan;

	// sin (n) seems to have trouble when n is very large, so we
	// keep phase in the range of 0-TWO_PI like this:
	while (phase > TWO_PI){
		phase -= TWO_PI;
	}

	if ( bNoise == true){
		// ---------------------- noise --------------
		for (size_t i = 0; i < buffer.getNumFrames(); i++){
			lAudio[i] = buffer[i*buffer.getNumChannels()    ] = ofRandom(0, 1) * volume * leftScale;
			rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = ofRandom(0, 1) * volume * rightScale;
		}
	} else {
		phaseAdder = 0.95f * phaseAdder + 0.05f * phaseAdderTarget;
		for (size_t i = 0; i < buffer.getNumFrames(); i++){
			phase += phaseAdder;
			float sample = sin(phase);
			lAudio[i] = buffer[i*buffer.getNumChannels()    ] = sample * volume * leftScale;
			rAudio[i] = buffer[i*buffer.getNumChannels() + 1] = sample * volume * rightScale;
		}
	}
	projectM.audio(&buffer.getBuffer()[0], bufferSize, outputChannels);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) { 
}
