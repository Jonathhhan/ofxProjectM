#include "ofApp.h"

void ofApp::setup() {
	ofSetWindowTitle("ofxProjectM Simple VLC Example");
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(18, 18, 22);
	ofDisableArbTex();
	gui.setup(nullptr, true, ImGuiConfigFlags_None, true);
	ImGui::GetIO().IniFilename = "imgui_projectm_vlc.ini";

	setupProjectM();
	setupPlayer();
	setupSoundStream();
	loadSeedMedia();
}

void ofApp::setupProjectM() {
	updateProjectMWindowSize();
	projectM.init();
	projectM.setPresetDuration(20.0);

	const int maxSamples = projectM.getMaxSamples();
	if (maxSamples > 0) {
		bufferSize = std::max(256, std::min(1024, maxSamples));
	}
}

void ofApp::setupPlayer() {
	player.setAudioCaptureEnabled(true);
	player.setAudioCaptureSampleFormat(ofxVlc4::AudioCaptureSampleFormat::Float32);
	player.setAudioCaptureSampleRate(sampleRate);
	player.setAudioCaptureChannelCount(outputChannels);
	player.setAudioCaptureBufferSeconds(1.0);
	player.init(0, nullptr);
}

void ofApp::setupSoundStream() {
	ofSoundStreamSettings settings;
	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = outputChannels;
	settings.numInputChannels = 0;
	settings.bufferSize = bufferSize;
	soundStream.setup(settings);
}

void ofApp::loadSeedMedia() {
	const std::vector<std::filesystem::path> candidates = {
		ofToDataPath("movie.mp4", true),
		ofToDataPath("sample.mp4", true)
	};

	for (const auto & candidate : candidates) {
		if (std::filesystem::exists(candidate)) {
			loadMedia(candidate, true);
			return;
		}
	}
}

bool ofApp::loadMedia(const std::filesystem::path & path, bool playNow) {
	if (path.empty() || !std::filesystem::exists(path)) {
		return false;
	}

	player.clearPlaylist();
	const int addedCount = player.addPathToPlaylist(path.string());
	if (addedCount <= 0 || !player.hasPlaylist()) {
		return false;
	}

	if (playNow) {
		player.playIndex(0);
	}

	return true;
}

void ofApp::update() {
	player.update();
	updateProjectMTextureBinding();
	projectM.update();
}

void ofApp::updateProjectMTextureBinding() {
	if (!useVideoTextureForProjectM) {
		projectM.useInternalTextureOnly();
		return;
	}

	ofTexture & texture = player.getRenderTexture();
	if (texture.isAllocated()) {
		projectM.setTexture(texture);
	} else {
		projectM.useInternalTextureOnly();
	}
}

void ofApp::draw() {
	const float panelY = previewMargin;
	const float panelWidth = (ofGetWidth() - previewMargin * 3.0f) * 0.5f;
	const float panelHeight = ofGetHeight() - panelY - previewMargin;

	ofNoFill();
	ofSetColor(90);
	ofDrawRectangle(previewMargin, panelY, panelWidth, panelHeight);
	ofDrawRectangle(previewMargin * 2.0f + panelWidth, panelY, panelWidth, panelHeight);

	ofSetColor(245);
	ofDrawBitmapString("libVLC video", previewMargin + 8.0f, panelY + 18.0f);
	ofDrawBitmapString("projectM", previewMargin * 2.0f + panelWidth + 8.0f, panelY + 18.0f);

	player.draw(previewMargin, panelY, panelWidth, panelHeight);
	projectM.draw(
		static_cast<int>(previewMargin * 2.0f + panelWidth),
		static_cast<int>(panelY),
		static_cast<int>(panelWidth),
		static_cast<int>(panelHeight));

	gui.begin();
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(360.0f, 260.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("ProjectM + VLC")) {
		ImGui::TextWrapped("libVLC drives both playback and the audio-reactive projectM output.");
		ImGui::Separator();
		ImGui::Text("Media: %s", mediaStatusLabel().c_str());
		ImGui::Text("Preset: %s", projectM.getPresetName().c_str());
		ImGui::Text("Texture source: %s", projectMTextureModeLabel().c_str());

		if (ImGui::Button("Open Media")) {
			const ofFileDialogResult result = ofSystemLoadDialog("Select a media file");
			if (result.bSuccess) {
				loadMedia(result.getPath(), true);
			}
		}

		if (!player.getPlaylist().empty()) {
			ImGui::SameLine();
			if (player.isPlaying()) {
				if (ImGui::Button("Pause")) {
					player.pause();
				}
			} else {
				if (ImGui::Button("Play")) {
					player.play();
				}
			}
		}

		if (ImGui::Button("Previous Preset")) {
			projectM.previousPreset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Next Preset")) {
			projectM.nextPreset();
		}
		ImGui::SameLine();
		if (ImGui::Button("Random Preset")) {
			projectM.randomPreset();
		}

		bool useTexture = useVideoTextureForProjectM;
		if (ImGui::Checkbox("Use VLC Video Texture", &useTexture)) {
			useVideoTextureForProjectM = useTexture;
			updateProjectMTextureBinding();
		}

		if (!projectM.getLastErrorMessage().empty()) {
			ImGui::TextColored(ImVec4(1.0f, 0.47f, 0.47f, 1.0f), "%s", projectM.getLastErrorMessage().c_str());
		}

		ImGui::TextWrapped("Keyboard fallback: O open file, Space play/pause, Left/Right preset, N random preset, T toggle texture.");
	}
	ImGui::End();
	gui.end();
}

void ofApp::audioOut(ofSoundBuffer & buffer) {
	buffer.set(0);

	const ofxVlc4::AudioStateInfo audioState = player.getAudioStateInfo();
	if (!audioState.ready) {
		return;
	}

	player.readAudioIntoBuffer(buffer, 1.0f);
	if (!buffer.getBuffer().empty()) {
		projectM.audio(
			buffer.getBuffer().data(),
			static_cast<int>(buffer.getNumFrames()),
			static_cast<int>(buffer.getNumChannels()));
	}
}

void ofApp::keyPressed(int key) {
	if (key == 'o' || key == 'O') {
		const ofFileDialogResult result = ofSystemLoadDialog("Select a media file");
		if (result.bSuccess) {
			loadMedia(result.getPath(), true);
		}
		return;
	}

	if (key == ' ') {
		if (player.hasPlaylist()) {
			if (player.isPlaying()) {
				player.pause();
			} else {
				player.play();
			}
		}
		return;
	}

	if (key == 'n' || key == 'N') {
		projectM.randomPreset();
		return;
	}

	if (key == 't' || key == 'T') {
		useVideoTextureForProjectM = !useVideoTextureForProjectM;
		updateProjectMTextureBinding();
		return;
	}

	if (key == OF_KEY_LEFT) {
		projectM.previousPreset();
		return;
	}

	if (key == OF_KEY_RIGHT) {
		projectM.nextPreset();
	}
}

void ofApp::windowResized(int w, int h) {
	(void)w;
	(void)h;
	updateProjectMWindowSize();
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (!dragInfo.files.empty()) {
		loadMedia(std::filesystem::path(dragInfo.files.front()), true);
	}
}

void ofApp::updateProjectMWindowSize() {
	const int width = std::max(1, static_cast<int>((ofGetWidth() - previewMargin * 3.0f) * 0.5f));
	const int height = std::max(1, static_cast<int>(ofGetHeight() - previewMargin * 2.0f));
	projectM.setWindowSize(width, height);
}

std::string ofApp::mediaStatusLabel() const {
	if (!player.hasPlaylist()) {
		return "Drop a file, press O, or place movie.mp4 in bin/data.";
	}

	const auto currentItem = player.getCurrentPlaylistItemInfo();
	std::string label = currentItem.path.empty()
		? "Loaded media"
		: ofFilePath::getBaseName(currentItem.path);
	const ofxVlc4::PlaybackStateInfo playbackState = player.getPlaybackStateInfo();
	if (playbackState.playing) {
		label += " [playing]";
	} else if (playbackState.pauseRequested) {
		label += " [paused]";
	} else if (playbackState.stopped) {
		label += " [stopped]";
	}
	return label;
}

std::string ofApp::projectMTextureModeLabel() const {
	return useVideoTextureForProjectM ? "VLC video texture" : "projectM internal textures only";
}

void ofApp::exit() {
	soundStream.close();
	player.close();
	gui.exit();
}
