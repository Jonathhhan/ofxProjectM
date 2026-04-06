#include "ofxProjectM.h"
#include "ofxProjectMPlaylist.h"
#include "ofxProjectMRender.h"

#include <algorithm>
#include <atomic>
#include <vector>

namespace {
constexpr const char * kLogChannel = "ofxProjectM";
constexpr int kOfxProjectMAddonVersionMajor = 1;
constexpr int kOfxProjectMAddonVersionMinor = 0;
constexpr int kOfxProjectMAddonVersionPatch = 0;
constexpr const char * kOfxProjectMAddonVersionString = "1.0.1";
std::atomic<int> gLogLevel { static_cast<int>(OF_LOG_NOTICE) };
std::atomic<int> gProjectMRuntimeLogLevel { static_cast<int>(PROJECTM_LOG_LEVEL_INFO) };
std::atomic<bool> gProjectMRuntimeLogCallbackEnabled { true };
std::atomic<bool> gProjectMRuntimeLogCurrentThreadOnly { true };

bool shouldLog(ofLogLevel level) {
	const ofLogLevel configuredLevel = static_cast<ofLogLevel>(gLogLevel.load());
	return configuredLevel != OF_LOG_SILENT && level >= configuredLevel;
}

void destroyProjectMHandle(projectm_handle & projectMHandle) {
	if (!projectMHandle) {
		return;
	}

	projectm_destroy(projectMHandle);
	projectMHandle = nullptr;
}

}

ofxProjectMAddonVersionInfo ofxProjectM::getAddonVersionInfo() {
	return {
		kOfxProjectMAddonVersionMajor,
		kOfxProjectMAddonVersionMinor,
		kOfxProjectMAddonVersionPatch,
		kOfxProjectMAddonVersionString
	};
}

void ofxProjectM::applyRuntimeParameters() const {
	if (!projectMHandle) {
		return;
	}

	projectm_set_window_size(projectMHandle, static_cast<size_t>(windowWidth), static_cast<size_t>(windowHeight));
	projectm_set_mesh_size(projectMHandle, static_cast<size_t>(meshWidth), static_cast<size_t>(meshHeight));
	projectm_set_aspect_correction(projectMHandle, aspectCorrectionEnabled);
	projectm_set_fps(projectMHandle, fps);
	projectm_set_beat_sensitivity(projectMHandle, beatSensitivity);
	projectm_set_hard_cut_enabled(projectMHandle, hardCutEnabled);
	projectm_set_hard_cut_duration(projectMHandle, hardCutDuration);
	projectm_set_hard_cut_sensitivity(projectMHandle, hardCutSensitivity);
	projectm_set_soft_cut_duration(projectMHandle, softCutDuration);
	projectm_set_preset_locked(projectMHandle, presetLocked);
	projectm_set_preset_duration(projectMHandle, presetDuration);
	projectm_set_frame_time(projectMHandle, frameTime);
	projectm_set_preset_start_clean(projectMHandle, presetStartClean);
	projectm_set_easter_egg(projectMHandle, easterEgg);
	projectm_set_texel_offset(projectMHandle, texelOffsetX, texelOffsetY);
}

void ofxProjectM::connectProjectMCallbacks() const {
	if (!projectMHandle) {
		return;
	}

	auto * self = const_cast<ofxProjectM *>(this);
	projectm_set_texture_load_event_callback(projectMHandle, textureLoadEvent, self);
	projectm_set_preset_switch_failed_event_callback(projectMHandle, presetSwitchFailed, self);
	if (!projectMPlaylistHandle) {
		projectm_set_preset_switch_requested_event_callback(
			projectMHandle,
			presetSwitchRequestedCallback ? presetSwitchRequested : nullptr,
			self);
	}
}

void ofxProjectM::setLogLevel(ofLogLevel level) {
	gLogLevel.store(static_cast<int>(level));
	setProjectMRuntimeLogLevel(toProjectMRuntimeLogLevel(level), gProjectMRuntimeLogCurrentThreadOnly.load());
}

ofLogLevel ofxProjectM::getLogLevel() {
	return static_cast<ofLogLevel>(gLogLevel.load());
}

projectm_log_level ofxProjectM::toProjectMRuntimeLogLevel(ofLogLevel level) {
	switch (level) {
	case OF_LOG_VERBOSE:
		return PROJECTM_LOG_LEVEL_TRACE;
	case OF_LOG_NOTICE:
		return PROJECTM_LOG_LEVEL_INFO;
	case OF_LOG_WARNING:
		return PROJECTM_LOG_LEVEL_WARN;
	case OF_LOG_ERROR:
		return PROJECTM_LOG_LEVEL_ERROR;
	case OF_LOG_FATAL_ERROR:
		return PROJECTM_LOG_LEVEL_FATAL;
	case OF_LOG_SILENT:
		return PROJECTM_LOG_LEVEL_FATAL;
	default:
		return PROJECTM_LOG_LEVEL_DEBUG;
	}
}

ofLogLevel ofxProjectM::toOfLogLevel(projectm_log_level level) {
	switch (level) {
	case PROJECTM_LOG_LEVEL_TRACE:
	case PROJECTM_LOG_LEVEL_DEBUG:
		return OF_LOG_VERBOSE;
	case PROJECTM_LOG_LEVEL_INFO:
		return OF_LOG_NOTICE;
	case PROJECTM_LOG_LEVEL_WARN:
		return OF_LOG_WARNING;
	case PROJECTM_LOG_LEVEL_ERROR:
		return OF_LOG_ERROR;
	case PROJECTM_LOG_LEVEL_FATAL:
		return OF_LOG_FATAL_ERROR;
	case PROJECTM_LOG_LEVEL_NOTSET:
	default:
		return OF_LOG_NOTICE;
	}
}

std::vector<const char *> ofxProjectM::makeCStringView(const std::vector<std::string> & values) {
	std::vector<const char *> out;
	out.reserve(values.size());
	for (const auto & value : values) {
		out.push_back(value.c_str());
	}
	return out;
}

void ofxProjectM::projectMLogCallback(const char * message, projectm_log_level logLevel, void * userData) {
	(void)userData;
	if (!message || *message == '\0') {
		return;
	}

	switch (toOfLogLevel(logLevel)) {
	case OF_LOG_VERBOSE:
		logVerbose(message);
		break;
	case OF_LOG_WARNING:
		logWarning(message);
		break;
	case OF_LOG_ERROR:
	case OF_LOG_FATAL_ERROR:
		logError(message);
		break;
	case OF_LOG_NOTICE:
	default:
		logNotice(message);
		break;
	}
}

void ofxProjectM::setProjectMRuntimeLogLevel(projectm_log_level level, bool currentThreadOnly) {
	gProjectMRuntimeLogLevel.store(static_cast<int>(level));
	gProjectMRuntimeLogCurrentThreadOnly.store(currentThreadOnly);
	projectm_set_log_level(level, currentThreadOnly);
}

projectm_log_level ofxProjectM::getProjectMRuntimeLogLevel() {
	return static_cast<projectm_log_level>(gProjectMRuntimeLogLevel.load());
}

void ofxProjectM::setProjectMRuntimeLogCallbackEnabled(bool enabled, bool currentThreadOnly) {
	gProjectMRuntimeLogCallbackEnabled.store(enabled);
	gProjectMRuntimeLogCurrentThreadOnly.store(currentThreadOnly);
	projectm_set_log_callback(enabled ? projectMLogCallback : nullptr, currentThreadOnly, nullptr);
	if (enabled) {
		projectm_set_log_level(getProjectMRuntimeLogLevel(), currentThreadOnly);
	}
}

bool ofxProjectM::isProjectMRuntimeLogCallbackEnabled() {
	return gProjectMRuntimeLogCallbackEnabled.load();
}

ofxProjectMVersionInfo ofxProjectM::getVersionInfo() {
	ofxProjectMVersionInfo info;
	projectm_get_version_components(&info.major, &info.minor, &info.patch);

	char * versionString = projectm_get_version_string();
	if (versionString) {
		info.versionString = versionString;
		projectm_free_string(versionString);
	}

	char * vcsVersionString = projectm_get_vcs_version_string();
	if (vcsVersionString) {
		info.vcsVersionString = vcsVersionString;
		projectm_free_string(vcsVersionString);
	}

	return info;
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
	ofxProjectMPlaylistInternal::destroyPlaylistHandle(projectMPlaylistHandle);
	destroyProjectMHandle(projectMHandle);
}

void ofxProjectM::init() {
	ofSetRandomSeed(ofGetSystemTimeMillis());
	clearLastMessages();

	ofxProjectMPlaylistInternal::destroyPlaylistHandle(projectMPlaylistHandle);
	destroyProjectMHandle(projectMHandle);

	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
	ofxProjectMRenderInternal::clearAllocatedFbo(fbo);

	projectMHandle = projectm_create();
	if (!projectMHandle) {
		setError("projectM_create failed");
		presetName.clear();
		return;
	}

	projectm_set_log_callback(
		isProjectMRuntimeLogCallbackEnabled() ? projectMLogCallback : nullptr,
		gProjectMRuntimeLogCurrentThreadOnly.load(),
		nullptr);
	projectm_set_log_level(getProjectMRuntimeLogLevel(), gProjectMRuntimeLogCurrentThreadOnly.load());

	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	if (!projectMPlaylistHandle) {
		setError("projectM_playlist_create failed");
		destroyProjectMHandle(projectMHandle);
		presetName.clear();
		return;
	}

	applyRuntimeParameters();
	if (textureSearchPaths.empty()) {
		textureSearchPaths = { ofToDataPath("textures", true) };
	}
	auto texturePaths = makeCStringView(textureSearchPaths);
	projectm_set_texture_search_paths(
		projectMHandle,
		ofxProjectMPlaylistInternal::projectMCStringArrayData(texturePaths),
		texturePaths.size());
	connectProjectMCallbacks();
	connectPlaylistCallbacks();
	applyPlaylistParameters();
	reloadPresets();
	logNotice("ProjectM initialized.");
}

void ofxProjectM::setMeshSize(int x, int y) {
	const int clampedWidth = std::max(1, x);
	const int clampedHeight = std::max(1, y);
	if (clampedWidth == meshWidth && clampedHeight == meshHeight) {
		return;
	}

	meshWidth = clampedWidth;
	meshHeight = clampedHeight;
	if (projectMHandle) {
		projectm_set_mesh_size(projectMHandle, static_cast<size_t>(meshWidth), static_cast<size_t>(meshHeight));
	}
}

void ofxProjectM::getMeshSize(int & x, int & y) const {
	x = meshWidth;
	y = meshHeight;
}

void ofxProjectM::setPresetDuration(double duration) {
	const double clampedDuration = std::max(0.0, duration);
	if (clampedDuration == presetDuration) {
		return;
	}

	presetDuration = clampedDuration;
	if (projectMHandle) {
		projectm_set_preset_duration(projectMHandle, presetDuration);
	}
}

double ofxProjectM::getPresetDuration() const {
	return presetDuration;
}

void ofxProjectM::setFrameTime(double secondsSinceFirstFrame) {
	frameTime = secondsSinceFirstFrame;
	if (projectMHandle) {
		projectm_set_frame_time(projectMHandle, frameTime);
	}
}

void ofxProjectM::clearFrameTime() {
	setFrameTime(-1.0);
}

double ofxProjectM::getFrameTime() const {
	return frameTime;
}

double ofxProjectM::getLastFrameTime() const {
	return projectMHandle ? projectm_get_last_frame_time(projectMHandle) : frameTime;
}

void ofxProjectM::setFps(int value) {
	const int clampedFps = std::max(1, value);
	if (clampedFps == fps) {
		return;
	}

	fps = clampedFps;
	if (projectMHandle) {
		projectm_set_fps(projectMHandle, fps);
	}
}

int ofxProjectM::getFps() const {
	return fps;
}

void ofxProjectM::setAspectCorrectionEnabled(bool enabled) {
	if (aspectCorrectionEnabled == enabled) {
		return;
	}

	aspectCorrectionEnabled = enabled;
	if (projectMHandle) {
		projectm_set_aspect_correction(projectMHandle, aspectCorrectionEnabled);
	}
}

bool ofxProjectM::isAspectCorrectionEnabled() const {
	return aspectCorrectionEnabled;
}

void ofxProjectM::setBeatSensitivity(float sensitivity) {
	const float clampedSensitivity = std::max(0.0f, sensitivity);
	if (clampedSensitivity == beatSensitivity) {
		return;
	}

	beatSensitivity = clampedSensitivity;
	if (projectMHandle) {
		projectm_set_beat_sensitivity(projectMHandle, beatSensitivity);
	}
}

float ofxProjectM::getBeatSensitivity() const {
	return beatSensitivity;
}

void ofxProjectM::setTexelOffset(float x, float y) {
	texelOffsetX = x;
	texelOffsetY = y;
	if (projectMHandle) {
		projectm_set_texel_offset(projectMHandle, texelOffsetX, texelOffsetY);
	}
}

void ofxProjectM::getTexelOffset(float & x, float & y) const {
	if (projectMHandle) {
		projectm_get_texel_offset(projectMHandle, &x, &y);
		return;
	}

	x = texelOffsetX;
	y = texelOffsetY;
}

void ofxProjectM::setEasterEgg(float value) {
	easterEgg = std::max(0.0f, value);
	if (projectMHandle) {
		projectm_set_easter_egg(projectMHandle, easterEgg);
	}
}

float ofxProjectM::getEasterEgg() const {
	return projectMHandle ? projectm_get_easter_egg(projectMHandle) : easterEgg;
}

void ofxProjectM::setHardCutEnabled(bool enabled) {
	if (hardCutEnabled == enabled) {
		return;
	}

	hardCutEnabled = enabled;
	if (projectMHandle) {
		projectm_set_hard_cut_enabled(projectMHandle, hardCutEnabled);
	}
}

bool ofxProjectM::isHardCutEnabled() const {
	return hardCutEnabled;
}

void ofxProjectM::setHardCutDuration(double duration) {
	const double clampedDuration = std::max(0.0, duration);
	if (clampedDuration == hardCutDuration) {
		return;
	}

	hardCutDuration = clampedDuration;
	if (projectMHandle) {
		projectm_set_hard_cut_duration(projectMHandle, hardCutDuration);
	}
}

double ofxProjectM::getHardCutDuration() const {
	return hardCutDuration;
}

void ofxProjectM::setHardCutSensitivity(float sensitivity) {
	const float clampedSensitivity = std::max(0.0f, sensitivity);
	if (clampedSensitivity == hardCutSensitivity) {
		return;
	}

	hardCutSensitivity = clampedSensitivity;
	if (projectMHandle) {
		projectm_set_hard_cut_sensitivity(projectMHandle, hardCutSensitivity);
	}
}

float ofxProjectM::getHardCutSensitivity() const {
	return hardCutSensitivity;
}

void ofxProjectM::setSoftCutDuration(double duration) {
	const double clampedDuration = std::max(0.0, duration);
	if (clampedDuration == softCutDuration) {
		return;
	}

	softCutDuration = clampedDuration;
	if (projectMHandle) {
		projectm_set_soft_cut_duration(projectMHandle, softCutDuration);
	}
}

double ofxProjectM::getSoftCutDuration() const {
	return softCutDuration;
}

void ofxProjectM::setPresetStartClean(bool enabled) {
	presetStartClean = enabled;
	if (projectMHandle) {
		projectm_set_preset_start_clean(projectMHandle, presetStartClean);
	}
}

bool ofxProjectM::isPresetStartClean() const {
	return projectMHandle ? projectm_get_preset_start_clean(projectMHandle) : presetStartClean;
}

void ofxProjectM::setPresetLocked(bool locked) {
	if (presetLocked == locked) {
		return;
	}

	presetLocked = locked;
	if (projectMHandle) {
		projectm_set_preset_locked(projectMHandle, presetLocked);
	}
}

bool ofxProjectM::isPresetLocked() const {
	return presetLocked;
}
