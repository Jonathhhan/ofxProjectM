#include "ofxProjectM.h"

#include <algorithm>

namespace {
constexpr const char * kLogChannel = "ofxProjectM";
std::atomic<int> gLogLevel { static_cast<int>(OF_LOG_NOTICE) };

void clearAllocatedFbo(ofFbo & fbo) {
	if (!fbo.isAllocated()) {
		return;
	}

	fbo.begin();
	ofClear(0, 0, 0, 0);
	fbo.end();
}

bool shouldLog(ofLogLevel level) {
	const ofLogLevel configuredLevel = static_cast<ofLogLevel>(gLogLevel.load());
	return configuredLevel != OF_LOG_SILENT && level >= configuredLevel;
}
}

std::string ofxProjectM::formatPresetName(const char * presetPath) {
	if (!presetPath) {
		return "";
	}

	return ofFilePath::getBaseName(std::string(presetPath));
}

void ofxProjectM::setLogLevel(ofLogLevel level) {
	gLogLevel.store(static_cast<int>(level));
}

ofLogLevel ofxProjectM::getLogLevel() {
	return static_cast<ofLogLevel>(gLogLevel.load());
}

void ofxProjectM::logVerbose(const std::string & message) {
	if (!message.empty() && shouldLog(OF_LOG_VERBOSE)) {
		ofLogVerbose(kLogChannel) << message;
	}
}

void ofxProjectM::logError(const std::string & message) {
	if (!message.empty() && shouldLog(OF_LOG_ERROR)) {
		ofLogError(kLogChannel) << message;
	}
}

void ofxProjectM::logWarning(const std::string & message) {
	if (!message.empty() && shouldLog(OF_LOG_WARNING)) {
		ofLogWarning(kLogChannel) << message;
	}
}

void ofxProjectM::logNotice(const std::string & message) {
	if (!message.empty() && shouldLog(OF_LOG_NOTICE)) {
		ofLogNotice(kLogChannel) << message;
	}
}

void ofxProjectM::setStatus(const std::string & message) {
	lastStatusMessage = message;
	lastErrorMessage.clear();
}

void ofxProjectM::setError(const std::string & message) {
	lastErrorMessage = message;
	lastStatusMessage.clear();
	logError(message);
}

void ofxProjectM::clearLastMessages() {
	lastStatusMessage.clear();
	lastErrorMessage.clear();
}

ofxProjectM::~ofxProjectM() {
	if (projectMPlaylistHandle) {
		projectm_playlist_destroy(projectMPlaylistHandle);
		projectMPlaylistHandle = nullptr;
	}
	if (projectMHandle) {
		projectm_destroy(projectMHandle);
		projectMHandle = nullptr;
	}
}

void ofxProjectM::init() {
	ofSetRandomSeed(ofGetSystemTimeMillis());
	clearLastMessages();

	// Re-init tears down the previous engine first so callbacks never point at stale handles.
	if (projectMPlaylistHandle) {
		projectm_playlist_destroy(projectMPlaylistHandle);
		projectMPlaylistHandle = nullptr;
	}
	if (projectMHandle) {
		projectm_destroy(projectMHandle);
		projectMHandle = nullptr;
	}

	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
	clearAllocatedFbo(fbo);

	projectMHandle = projectm_create();
	if (!projectMHandle) {
		setError("projectM_create failed");
		presetName.clear();
		return;
	}

	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	if (!projectMPlaylistHandle) {
		setError("projectM_playlist_create failed");
		projectm_destroy(projectMHandle);
		projectMHandle = nullptr;
		presetName.clear();
		return;
	}

	projectm_set_window_size(projectMHandle, windowWidth, windowHeight);
	projectm_set_mesh_size(projectMHandle, 32, 32);
	projectm_set_aspect_correction(projectMHandle, true);
	projectm_set_fps(projectMHandle, 60);
	projectm_set_beat_sensitivity(projectMHandle, 1.0);
	projectm_set_hard_cut_enabled(projectMHandle, true);
	projectm_set_hard_cut_duration(projectMHandle, 10.0);
	projectm_set_hard_cut_sensitivity(projectMHandle, 1.0);
	projectm_set_soft_cut_duration(projectMHandle, 5.0);
	projectm_set_preset_locked(projectMHandle, false);
	projectm_set_preset_duration(projectMHandle, 30.0);
	const std::string textureSearchPath = ofToDataPath("textures", true);
	std::vector<const char *> textures = { textureSearchPath.c_str() };
	projectm_set_texture_search_paths(projectMHandle, textures.data(), 1);
	projectm_set_texture_load_event_callback(projectMHandle, textureLoadEvent, this);
	projectm_playlist_set_preset_switched_event_callback(projectMPlaylistHandle, presetSwitched, this);
	projectm_playlist_set_preset_switch_failed_event_callback(projectMPlaylistHandle, presetSwitchFailed, this);
	reloadPresets();
	logNotice("ProjectM initialized.");
}

void ofxProjectM::reloadPresets() {
	clearLastMessages();

	if (!projectMHandle) {
		setError("Cannot reload presets before projectM is initialized.");
		presetName.clear();
		return;
	}

	std::string currentPresetPath;
	uint32_t previousPosition = 0;
	if (projectMPlaylistHandle) {
		const auto previousPresetCount = projectm_playlist_size(projectMPlaylistHandle);
		if (previousPresetCount > 0) {
			previousPosition = projectm_playlist_get_position(projectMPlaylistHandle);
			char * currentItem = projectm_playlist_item(projectMPlaylistHandle, previousPosition);
			if (currentItem) {
				currentPresetPath = currentItem;
				projectm_playlist_free_string(currentItem);
			}
		}

		projectm_playlist_destroy(projectMPlaylistHandle);
		projectMPlaylistHandle = nullptr;
	}

	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	if (!projectMPlaylistHandle) {
		setError("projectM_playlist_create failed");
		presetName.clear();
		return;
	}

	projectm_playlist_set_preset_switched_event_callback(projectMPlaylistHandle, presetSwitched, this);
	projectm_playlist_set_preset_switch_failed_event_callback(projectMPlaylistHandle, presetSwitchFailed, this);

	const std::string presetPath = ofToDataPath("presets", true);
	projectm_playlist_add_path(projectMPlaylistHandle, presetPath.c_str(), true, false);
	projectm_playlist_set_retry_count(projectMPlaylistHandle, 0);
	projectm_playlist_set_shuffle(projectMPlaylistHandle, true);

	const auto presetCount = projectm_playlist_size(projectMPlaylistHandle);
	if (presetCount == 0) {
		presetName = "No presets loaded";
		setStatus("No projectM presets loaded.");
		return;
	}
	projectm_playlist_sort(projectMPlaylistHandle, 0, presetCount, SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);

	uint32_t targetPosition =
		!currentPresetPath.empty() ? std::min(previousPosition, presetCount - 1) : static_cast<uint32_t>(ofRandom(0, presetCount));
	if (!currentPresetPath.empty()) {
		for (uint32_t index = 0; index < presetCount; ++index) {
			char * candidateItem = projectm_playlist_item(projectMPlaylistHandle, index);
			if (!candidateItem) {
				continue;
			}

			const bool isMatch = currentPresetPath == candidateItem;
			projectm_playlist_free_string(candidateItem);
			if (isMatch) {
				targetPosition = index;
				break;
			}
		}
	}

	projectm_playlist_set_position(projectMPlaylistHandle, targetPosition, true);

	char * activeItem = projectm_playlist_item(projectMPlaylistHandle, projectm_playlist_get_position(projectMPlaylistHandle));
	if (activeItem) {
		presetName = formatPresetName(activeItem);
		projectm_playlist_free_string(activeItem);
	} else {
		presetName = "No presets loaded";
	}

	setStatus(currentPresetPath.empty() ? "Loaded projectM presets." : "Reloaded projectM presets.");
	logNotice(currentPresetPath.empty() ? "Presets loaded." : "Presets reloaded.");
}

void ofxProjectM::setTexture(const ofTexture & tex) {
	texture = tex;
}

void ofxProjectM::clearTexture() {
	texture.clear();
}

void ofxProjectM::useInternalTextureOnly() {
	clearTexture();
}

void ofxProjectM::resetTextures() const {
	if (projectMHandle) {
		projectm_reset_textures(projectMHandle);
	}
}

void ofxProjectM::textureLoadEvent(const char * texture_name, projectm_texture_load_data * data, void * user_data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(user_data);
	(void)texture_name;
	if (!that || !data) {
		return;
	}

	if (!that->texture.isAllocated()) {
		data->data = nullptr;
		data->width = 0;
		data->height = 0;
		data->texture_id = 0;
		return;
	}

	data->texture_id = that->texture.getTextureData().textureID;
	data->width = that->texture.getWidth();
	data->height = that->texture.getHeight();
}

void ofxProjectM::presetSwitched(bool hardCut, unsigned int index, void * data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(data);
	if (!that || !that->projectMPlaylistHandle) {
		return;
	}

	char * presetPath = projectm_playlist_item(that->projectMPlaylistHandle, index);
	that->presetName = formatPresetName(presetPath);
	if (presetPath) {
		projectm_playlist_free_string(presetPath);
	}
	that->setStatus("Switched projectM preset.");
	if (!that->presetName.empty()) {
		that->logNotice("Preset switched: " + that->presetName + ".");
	} else {
		that->logNotice("Preset switched.");
	}
}

void ofxProjectM::presetSwitchFailed(const char * presetFilename, const char * message, void * data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(data);
	if (!that) {
		return;
	}

	that->presetName = std::string("Preset load failed: ") + formatPresetName(presetFilename);
	if (message) {
		that->setError(message);
	}
}

void ofxProjectM::setWindowSize(int x, int y) {
	if (x == windowWidth && y == windowHeight && fbo.isAllocated()) {
		return;
	}

	windowWidth = x;
	windowHeight = y;
	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
	clearAllocatedFbo(fbo);
	if (projectMHandle) {
		projectm_set_window_size(projectMHandle, windowWidth, windowHeight);
	}
}

void ofxProjectM::setMeshSize(int x, int y) const {
	if (projectMHandle) {
		projectm_set_mesh_size(projectMHandle, x, y);
	}
}

void ofxProjectM::setPresetDuration(double duration) const {
	if (projectMHandle) {
		projectm_set_preset_duration(projectMHandle, duration);
	}
}

void ofxProjectM::update() {
	if (!projectMHandle || !fbo.isAllocated()) {
		return;
	}

	ofPushStyle();
	fbo.bind();
	projectm_opengl_render_frame_fbo(projectMHandle, fbo.getId());
	fbo.unbind();
	ofPopStyle();
}

void ofxProjectM::draw(int x, int y) {
	if (fbo.isAllocated()) {
		fbo.getTexture().draw(x, y);
	}
}

void ofxProjectM::draw(int x, int y, int a, int b) {
	if (fbo.isAllocated()) {
		fbo.getTexture().draw(x, y, a, b);
	}
}

const ofTexture & ofxProjectM::getTexture() const {
	if (fbo.isAllocated()) {
		return fbo.getTexture();
	}

	return texture;
}

void ofxProjectM::bind() {
	if (fbo.isAllocated()) {
		fbo.getTexture().bind();
	}
}

void ofxProjectM::unbind() {
	if (fbo.isAllocated()) {
		fbo.getTexture().unbind();
	}
}

void ofxProjectM::previousPreset() const {
	if (projectMPlaylistHandle) {
		projectm_playlist_play_previous(projectMPlaylistHandle, true);
	}
}

void ofxProjectM::nextPreset() const {
	if (projectMPlaylistHandle) {
		projectm_playlist_play_next(projectMPlaylistHandle, true);
	}
}

void ofxProjectM::randomPreset() const {
	if (!projectMPlaylistHandle) {
		return;
	}

	const auto presetCount = projectm_playlist_size(projectMPlaylistHandle);
	if (presetCount > 0) {
		projectm_playlist_set_position(projectMPlaylistHandle, ofRandom(0, presetCount), true);
	}
}

int ofxProjectM::getPresetCount() const {
	if (!projectMPlaylistHandle) {
		return 0;
	}

	return static_cast<int>(projectm_playlist_size(projectMPlaylistHandle));
}

int ofxProjectM::getPresetIndex() const {
	if (!projectMPlaylistHandle || projectm_playlist_size(projectMPlaylistHandle) == 0) {
		return -1;
	}

	return static_cast<int>(projectm_playlist_get_position(projectMPlaylistHandle));
}

bool ofxProjectM::setPresetIndex(int index, bool hardCut) const {
	if (!projectMPlaylistHandle) {
		return false;
	}

	const auto presetCount = projectm_playlist_size(projectMPlaylistHandle);
	if (presetCount == 0 || index < 0 || index >= static_cast<int>(presetCount)) {
		return false;
	}

	projectm_playlist_set_position(projectMPlaylistHandle, static_cast<uint32_t>(index), hardCut);
	return true;
}

const std::string & ofxProjectM::getPresetName() const {
	return presetName;
}

int ofxProjectM::getMaxSamples() const {
	return projectm_pcm_get_max_samples();
}

void ofxProjectM::audio(const float * buffer, int bufferSize, int channels) const {
	if (projectMHandle && buffer && bufferSize > 0 && channels > 0) {
		projectm_pcm_add_float(projectMHandle, buffer, bufferSize, (projectm_channels)channels);
	}
}
