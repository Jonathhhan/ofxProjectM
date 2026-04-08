#pragma once

#include "ofMain.h"
#include "projectM-4/projectM.h"
#include "projectM-4/playlist.h"
#include "projectM-4/types.h"

#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

struct ofxProjectMVersionInfo {
	int major = 0;
	int minor = 0;
	int patch = 0;
	std::string versionString;
	std::string vcsVersionString;
};

struct ofxProjectMAddonVersionInfo {
	int major = 0;
	int minor = 0;
	int patch = 0;
	std::string versionString;
};

enum class ofxProjectMPlaylistSortPredicate {
	FullPath = SORT_PREDICATE_FULL_PATH,
	FileNameOnly = SORT_PREDICATE_FILENAME_ONLY
};

enum class ofxProjectMPlaylistSortOrder {
	Ascending = SORT_ORDER_ASCENDING,
	Descending = SORT_ORDER_DESCENDING
};

// Waveform render types used in touch events.
enum class ofxProjectMTouchType {
	Random = PROJECTM_TOUCH_TYPE_RANDOM,
	Circle = PROJECTM_TOUCH_TYPE_CIRCLE,
	RadialBlob = PROJECTM_TOUCH_TYPE_RADIAL_BLOB,
	Blob2 = PROJECTM_TOUCH_TYPE_BLOB2,
	Blob3 = PROJECTM_TOUCH_TYPE_BLOB3,
	DerivativeLine = PROJECTM_TOUCH_TYPE_DERIVATIVE_LINE,
	Blob5 = PROJECTM_TOUCH_TYPE_BLOB5,
	Line = PROJECTM_TOUCH_TYPE_LINE,
	DoubleLine = PROJECTM_TOUCH_TYPE_DOUBLE_LINE
};

class ofxProjectM {
public:
	~ofxProjectM();
	static ofxProjectMAddonVersionInfo getAddonVersionInfo();
	static void setLogLevel(ofLogLevel level);
	static ofLogLevel getLogLevel();
	static void setProjectMRuntimeLogLevel(projectm_log_level level, bool currentThreadOnly = true);
	static projectm_log_level getProjectMRuntimeLogLevel();
	static void setProjectMRuntimeLogCallbackEnabled(bool enabled, bool currentThreadOnly = true);
	static bool isProjectMRuntimeLogCallbackEnabled();
	static ofxProjectMVersionInfo getVersionInfo();
	static void logVerbose(const std::string & message);
	static void logError(const std::string & message);
	static void logWarning(const std::string & message);
	static void logNotice(const std::string & message);
	// init() creates the projectM engine, default render target, and preset playlist.
	void init();
	void setTexture(const ofTexture & texture);
	void clearTexture();
	void useInternalTextureOnly();
	void resetTextures() const;
	void setWindowSize(int x, int y);
	void getWindowSize(int & x, int & y) const;
	void setMeshSize(int x, int y);
	void getMeshSize(int & x, int & y) const;
	void setTextureSearchPaths(const std::vector<std::string> & searchPaths);
	const std::vector<std::string> & getTextureSearchPaths() const;
	void writeDebugImageOnNextFrame(const std::string & outputPath) const;
	void setPresetDuration(double duration);
	double getPresetDuration() const;
	void setFrameTime(double secondsSinceFirstFrame);
	void clearFrameTime();
	double getFrameTime() const;
	double getLastFrameTime() const;
	void setFps(int fps);
	int getFps() const;
	void setAspectCorrectionEnabled(bool enabled);
	bool isAspectCorrectionEnabled() const;
	void setBeatSensitivity(float sensitivity);
	float getBeatSensitivity() const;
	void setTexelOffset(float x, float y);
	void getTexelOffset(float & x, float & y) const;
	void setEasterEgg(float value);
	float getEasterEgg() const;
	void setHardCutEnabled(bool enabled);
	bool isHardCutEnabled() const;
	void setHardCutDuration(double duration);
	double getHardCutDuration() const;
	void setHardCutSensitivity(float sensitivity);
	float getHardCutSensitivity() const;
	void setSoftCutDuration(double duration);
	double getSoftCutDuration() const;
	void setPresetStartClean(bool enabled);
	bool isPresetStartClean() const;
	void setPresetLocked(bool locked);
	bool isPresetLocked() const;
	void reloadPresets();
	bool loadPresetFile(const std::string & presetPath, bool smoothTransition = true);
	bool loadPresetData(const std::string & presetData, bool smoothTransition = true);
	void setShuffleEnabled(bool enabled);
	bool isShuffleEnabled() const;
	void setRetryCount(std::uint32_t retryCount);
	std::uint32_t getRetryCount() const;
	void clearPlaylist();
	std::vector<std::string> getPlaylistItems(
		std::uint32_t start = 0,
		std::uint32_t count = std::numeric_limits<std::uint32_t>::max()) const;
	std::string getPlaylistItemPath(std::uint32_t index) const;
	std::uint32_t addPresetPath(const std::string & path, bool recurseSubdirs = true, bool allowDuplicates = false);
	std::uint32_t insertPresetPath(
		const std::string & path,
		std::uint32_t index,
		bool recurseSubdirs = true,
		bool allowDuplicates = false);
	bool addPreset(const std::string & presetPath, bool allowDuplicates = false);
	bool insertPreset(const std::string & presetPath, std::uint32_t index, bool allowDuplicates = false);
	std::uint32_t addPresets(const std::vector<std::string> & presetPaths, bool allowDuplicates = false);
	std::uint32_t insertPresets(
		const std::vector<std::string> & presetPaths,
		std::uint32_t index,
		bool allowDuplicates = false);
	bool removePreset(std::uint32_t index);
	std::uint32_t removePresets(std::uint32_t index, std::uint32_t count);
	void sortPlaylist(
		std::uint32_t startIndex,
		std::uint32_t count,
		ofxProjectMPlaylistSortPredicate predicate = ofxProjectMPlaylistSortPredicate::FullPath,
		ofxProjectMPlaylistSortOrder order = ofxProjectMPlaylistSortOrder::Ascending);
	void setPlaylistFilter(const std::vector<std::string> & filters);
	const std::vector<std::string> & getPlaylistFilter() const;
	std::size_t applyPlaylistFilter();
	int playLastPreset(bool hardCut = true) const;
	void setPresetSwitchRequestedCallback(const std::function<void(bool)> & callback);
	void clearPresetSwitchRequestedCallback();
	void setPlaylistPresetLoadCallback(
		const std::function<bool(unsigned int, const std::string &, bool)> & callback);
	void clearPlaylistPresetLoadCallback();
	void update();
	void draw(int x, int y);
	void draw(int x, int y, int a, int b);
	const ofTexture & getTexture() const;
	void bind();
	void unbind();
	void audio(const float * buffer, int bufferSize, int channels) const;
	void audio(const int16_t * buffer, int bufferSize, int channels) const;
	void audio(const uint8_t * buffer, int bufferSize, int channels) const;
	void touch(float x, float y, int pressure = 1, projectm_touch_type type = PROJECTM_TOUCH_TYPE_RANDOM) const;
	void touchDrag(float x, float y, int pressure = 1) const;
	void touchDestroy(float x, float y) const;
	void clearTouches() const;
	uint32_t createSprite(const std::string & type, const std::string & code) const;
	void destroySprite(uint32_t spriteId) const;
	void clearSprites() const;
	uint32_t getSpriteCount() const;
	std::vector<uint32_t> getSpriteIds() const;
	void setMaxSprites(uint32_t maxSprites) const;
	uint32_t getMaxSprites() const;
	void previousPreset() const;
	void nextPreset() const;
	void randomPreset() const;
	bool restartPreset(bool hardCut = true) const;
	int getPresetCount() const;
	int getPresetIndex() const;
	bool setPresetIndex(int index, bool hardCut = true) const;
	const std::string & getPresetName() const;
	int getMaxSamples() const;
	const std::string & getLastStatusMessage() const { return lastStatusMessage; }
	const std::string & getLastErrorMessage() const { return lastErrorMessage; }
	void clearLastMessages();
	static void textureLoadEvent(const char * textureName, projectm_texture_load_data * data, void * userData);
	static void presetSwitchRequested(bool hardCut, void * data);
	static void presetSwitched(bool hardCut, unsigned int index, void* data);
	static void presetSwitchFailed(const char* presetFilename, const char* message, void* data);
	static bool playlistPresetLoadRequested(unsigned int index, const char * filename, bool hardCut, void * data);
private:
	static projectm_log_level toProjectMRuntimeLogLevel(ofLogLevel level);
	static ofLogLevel toOfLogLevel(projectm_log_level level);
	static std::vector<const char *> makeCStringView(const std::vector<std::string> & values);
	static void projectMLogCallback(const char * message, projectm_log_level logLevel, void * userData);
	void applyRuntimeParameters() const;
	void applyPlaylistParameters() const;
	void connectProjectMCallbacks() const;
	void connectPlaylistCallbacks() const;
	void syncPresetNameFromPlaylist();
	void setStatus(const std::string & message);
	void setError(const std::string & message);
	projectm_handle projectMHandle = nullptr;
	projectm_playlist_handle projectMPlaylistHandle = nullptr;
	ofTexture texture;
	ofFbo fbo;
	int windowWidth = 1024;
	int windowHeight = 1024;
	int meshWidth = 32;
	int meshHeight = 32;
	int fps = 60;
	bool aspectCorrectionEnabled = true;
	float beatSensitivity = 1.0f;
	bool hardCutEnabled = true;
	double hardCutDuration = 10.0;
	float hardCutSensitivity = 1.0f;
	double softCutDuration = 5.0;
	double presetDuration = 30.0;
	double frameTime = -1.0;
	std::vector<std::string> textureSearchPaths;
	bool presetLocked = false;
	bool presetStartClean = false;
	bool shuffleEnabled = true;
	std::uint32_t retryCount = 0;
	float easterEgg = 1.0f;
	float texelOffsetX = 0.0f;
	float texelOffsetY = 0.0f;
	std::vector<std::string> playlistFilter;
	std::function<void(bool)> presetSwitchRequestedCallback;
	std::function<bool(unsigned int, const std::string &, bool)> playlistPresetLoadCallback;
	std::string presetName;
	std::string lastStatusMessage;
	std::string lastErrorMessage;
};
