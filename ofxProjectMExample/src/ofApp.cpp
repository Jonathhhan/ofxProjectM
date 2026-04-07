#include "ofApp.h"

#include <set>

namespace {
std::vector<std::filesystem::path> defaultSeedMediaCandidates() {
	const std::filesystem::path sharedMoviesDirectory =
		std::filesystem::path(ofFilePath::getCurrentExeDir()) /
		"..\\..\\..\\..\\examples\\video\\videoPlayerExample\\bin\\data\\movies";

	return {
		ofToDataPath("finger.mp4", true),
		ofToDataPath("fingers.mp4", true),
		ofToDataPath("movie.mp4", true),
		ofToDataPath("sample.mp4", true),
		sharedMoviesDirectory / "finger.mp4",
		sharedMoviesDirectory / "fingers.mp4"
	};
}

bool isLikelySupportedMediaPath(const std::filesystem::path & path) {
	static const std::set<std::string> extensions = {
		".wav", ".mp3", ".flac", ".ogg", ".opus",
		".m4a", ".aac", ".aiff", ".wma", ".mid", ".midi",
		".mp4", ".mov", ".mkv", ".avi", ".wmv", ".asf",
		".webm", ".m4v", ".mpg", ".mpeg", ".ts", ".mts",
		".m2ts", ".m2v", ".vob", ".ogv", ".3gp", ".m3u8",
		".jpg", ".jpeg", ".png", ".bmp", ".webp", ".tif", ".tiff"
	};
	const std::string extension = ofToLower(path.extension().string());
	return !extension.empty() && extensions.count(extension) > 0;
}
}

void ofApp::setup() {
	ofSetWindowTitle("ofxProjectMExample");
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(18, 18, 22);
	ofDisableArbTex();

	gui.setup(nullptr, true, ImGuiConfigFlags_None, true);
	ImGui::GetIO().IniFilename = "imgui_projectm.ini";

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
	for (const auto & candidate : defaultSeedMediaCandidates()) {
		if (std::filesystem::exists(candidate)) {
			replacePlaylistWithPath(candidate, true);
			return;
		}
	}
}

bool ofApp::replacePlaylistWithPath(const std::filesystem::path & path, bool playNow) {
	if (path.empty() || !std::filesystem::exists(path) || !isLikelySupportedMediaPath(path)) {
		return false;
	}

	player.clearPlaylist();
	selectedPlaylistIndex = -1;

	const int addedCount = player.addPathToPlaylist(path.string());
	if (addedCount <= 0 || !player.hasPlaylist()) {
		return false;
	}

	selectedPlaylistIndex = 0;
	if (playNow) {
		player.playIndex(0);
	}

	return true;
}

bool ofApp::addPathToPlaylist(const std::filesystem::path & path) {
	if (path.empty() || !std::filesystem::exists(path) || !isLikelySupportedMediaPath(path)) {
		return false;
	}

	const bool hadPlaylist = player.hasPlaylist();
	const int addedCount = player.addPathToPlaylist(path.string());
	if (addedCount <= 0 || !player.hasPlaylist()) {
		return false;
	}

	const auto state = player.getPlaylistStateInfo();
	if (!hadPlaylist && state.size > 0) {
		selectedPlaylistIndex = 0;
		player.playIndex(0);
	} else if (selectedPlaylistIndex < 0 && state.size > 0) {
		selectedPlaylistIndex = state.currentIndex >= 0 ? state.currentIndex : 0;
	}

	return true;
}

void ofApp::addDroppedPathsToPlaylist(const std::vector<std::filesystem::path> & paths) {
	bool addedAny = false;
	for (const auto & path : paths) {
		addedAny = addPathToPlaylist(path) || addedAny;
	}

	if (!addedAny && !paths.empty()) {
		replacePlaylistWithPath(paths.front(), true);
	}
}

void ofApp::update() {
	player.update();
	updateProjectMTextureBinding();
	projectM.update();
}

void ofApp::draw() {
	const float top = previewMargin;
	const float previewWidth = (ofGetWidth() - previewMargin * 3.0f) * 0.5f;
	const float previewHeight = ofGetHeight() - top - previewMargin;
	const float rightX = previewMargin * 2.0f + previewWidth;

	ofNoFill();
	ofSetColor(90);
	ofDrawRectangle(previewMargin, top, previewWidth, previewHeight);
	ofDrawRectangle(rightX, top, previewWidth, previewHeight);

	ofSetColor(245);
	ofDrawBitmapString("libVLC video", previewMargin + 8.0f, top + 18.0f);
	ofDrawBitmapString("projectM", rightX + 8.0f, top + 18.0f);

	player.draw(previewMargin, top, previewWidth, previewHeight);
	projectM.draw(static_cast<int>(rightX), static_cast<int>(top), static_cast<int>(previewWidth), static_cast<int>(previewHeight));

	const auto playlistState = player.getPlaylistStateInfo();
	clampSelectedPlaylistIndex(playlistState);

	gui.begin();
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(390.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("ProjectM + VLC")) {
		ImGui::TextWrapped("A compact projectM example driven by libVLC playback, with one media view, one projectM view, and a simple playlist.");
		ImGui::Separator();
		ImGui::Text("Media: %s", mediaStatusLabel().c_str());
		ImGui::Text("Preset: %s", projectM.getPresetName().c_str());
		ImGui::Text("Texture source: %s", projectMTextureModeLabel().c_str());

		if (ImGui::Button("Open Replace")) {
			ofFileDialogResult result = ofSystemLoadDialog("Select a media file");
			if (result.bSuccess) {
				replacePlaylistWithPath(result.getPath(), true);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Media")) {
			ofFileDialogResult result = ofSystemLoadDialog("Add a media file");
			if (result.bSuccess) {
				addPathToPlaylist(result.getPath());
			}
		}

		if (ImGui::Button(player.isPlaying() ? "Pause" : "Play")) {
			if (player.hasPlaylist()) {
				if (player.isPlaying()) {
					player.pause();
				} else if (playlistState.currentIndex >= 0) {
					player.playIndex(playlistState.currentIndex);
				} else {
					player.play();
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop")) {
			player.stop();
		}
		ImGui::SameLine();
		if (ImGui::Button("Prev Item")) {
			player.previousMediaListItem();
		}
		ImGui::SameLine();
		if (ImGui::Button("Next Item")) {
			player.nextMediaListItem();
		}

		ImGui::Separator();
		ImGui::Text("Playlist");
		ImGui::BeginChild("PlaylistList", ImVec2(0.0f, 180.0f), true);
		for (const auto & item : playlistState.items) {
			const std::string label = item.index == playlistState.currentIndex
				? ("> " + item.label)
				: item.label;
			if (ImGui::Selectable(label.c_str(), selectedPlaylistIndex == item.index)) {
				selectedPlaylistIndex = item.index;
				player.playIndex(item.index);
			}
		}
		ImGui::EndChild();

		if (ImGui::Button("Remove Selected")) {
			if (selectedPlaylistIndex >= 0 && selectedPlaylistIndex < playlistState.size) {
				player.removeFromPlaylist(selectedPlaylistIndex);
				const auto updatedState = player.getPlaylistStateInfo();
				clampSelectedPlaylistIndex(updatedState);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Playlist")) {
			player.clearPlaylist();
			selectedPlaylistIndex = -1;
		}

		ImGui::Separator();
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

		ImGui::Separator();
		ImGui::TextWrapped("Keyboard fallback: O open/replace, A add media, Space play/pause, S stop, Up/Down playlist, Delete remove selected, Left/Right preset, N random preset, T toggle texture.");
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
		ofFileDialogResult result = ofSystemLoadDialog("Select a media file");
		if (result.bSuccess) {
			replacePlaylistWithPath(result.getPath(), true);
		}
		return;
	}

	if (key == 'a' || key == 'A') {
		ofFileDialogResult result = ofSystemLoadDialog("Add a media file");
		if (result.bSuccess) {
			addPathToPlaylist(result.getPath());
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

	if (key == 's' || key == 'S') {
		player.stop();
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
		return;
	}

	if (key == OF_KEY_UP) {
		const auto state = player.getPlaylistStateInfo();
		if (state.size > 0) {
			selectedPlaylistIndex = std::max(0, selectedPlaylistIndex - 1);
			player.playIndex(selectedPlaylistIndex);
		}
		return;
	}

	if (key == OF_KEY_DOWN) {
		const auto state = player.getPlaylistStateInfo();
		if (state.size > 0) {
			selectedPlaylistIndex = std::min<int>(static_cast<int>(state.size) - 1, selectedPlaylistIndex + 1);
			player.playIndex(selectedPlaylistIndex);
		}
		return;
	}

	if (key == OF_KEY_DEL || key == OF_KEY_BACKSPACE) {
		const auto state = player.getPlaylistStateInfo();
		if (selectedPlaylistIndex >= 0 && selectedPlaylistIndex < state.size) {
			player.removeFromPlaylist(selectedPlaylistIndex);
			clampSelectedPlaylistIndex(player.getPlaylistStateInfo());
		}
	}
}

void ofApp::windowResized(int w, int h) {
	(void)w;
	(void)h;
	updateProjectMWindowSize();
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (!dragInfo.files.empty()) {
		addDroppedPathsToPlaylist(dragInfo.files);
	}
}

void ofApp::exit() {
	soundStream.close();
	player.close();
	gui.exit();
}

std::string ofApp::mediaStatusLabel() const {
	if (!player.hasPlaylist()) {
		return "Open media or place fingers.mp4 in bin/data.";
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

void ofApp::updateProjectMWindowSize() {
	const int width = std::max(1, static_cast<int>((ofGetWidth() - previewMargin * 3.0f) * 0.5f));
	const int height = std::max(1, static_cast<int>(ofGetHeight() - previewMargin * 2.0f));
	projectM.setWindowSize(width, height);
}

void ofApp::clampSelectedPlaylistIndex(const ofxVlc4::PlaylistStateInfo & state) {
	if (state.size <= 0) {
		selectedPlaylistIndex = -1;
		return;
	}

	if (state.currentIndex >= 0) {
		selectedPlaylistIndex = state.currentIndex;
		return;
	}

	selectedPlaylistIndex = ofClamp(selectedPlaylistIndex, 0, state.size - 1);
}
