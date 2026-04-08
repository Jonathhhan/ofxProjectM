#include "ofxProjectM.h"
#include "ofxProjectMPlaylist.h"

#include <algorithm>
#include <cstddef>
#include <vector>

std::string ofxProjectMPlaylistInternal::formatPresetName(const char * presetPath) {
	if (!presetPath) {
		return "";
	}

	return ofFilePath::getBaseName(std::string(presetPath));
}

std::string ofxProjectMPlaylistInternal::copyPlaylistString(char * value) {
	if (!value) {
		return "";
	}

	const std::string copied = value;
	projectm_playlist_free_string(value);
	return copied;
}

const char ** ofxProjectMPlaylistInternal::projectMCStringArrayData(std::vector<const char *> & values) {
	return values.empty() ? nullptr : const_cast<const char **>(values.data());
}

void ofxProjectMPlaylistInternal::destroyPlaylistHandle(projectm_playlist_handle & playlistHandle) {
	if (!playlistHandle) {
		return;
	}

	projectm_playlist_destroy(playlistHandle);
	playlistHandle = nullptr;
}

void ofxProjectM::applyPlaylistParameters() const {
	if (!projectMPlaylistHandle) {
		return;
	}

	projectm_playlist_set_retry_count(projectMPlaylistHandle, retryCount);
	projectm_playlist_set_shuffle(projectMPlaylistHandle, shuffleEnabled);
	if (playlistFilter.empty()) {
		projectm_playlist_set_filter(projectMPlaylistHandle, nullptr, 0);
		return;
	}

	auto filters = makeCStringView(playlistFilter);
	projectm_playlist_set_filter(
		projectMPlaylistHandle,
		ofxProjectMPlaylistInternal::projectMCStringArrayData(filters),
		filters.size());
}

void ofxProjectM::connectPlaylistCallbacks() {
	if (!projectMPlaylistHandle) {
		return;
	}

	auto * self = const_cast<ofxProjectM *>(this);
	projectm_playlist_set_preset_switched_event_callback(projectMPlaylistHandle, presetSwitched, self);
	projectm_playlist_set_preset_switch_failed_event_callback(projectMPlaylistHandle, presetSwitchFailed, self);
	projectm_playlist_set_preset_load_event_callback(
		projectMPlaylistHandle,
		(presetSwitchRequestedCallback || playlistPresetLoadCallback) ? playlistPresetLoadRequested : nullptr,
		self);
}

void ofxProjectM::syncPresetNameFromPlaylist() {
	if (!projectMPlaylistHandle || projectm_playlist_size(projectMPlaylistHandle) == 0) {
		presetName = "No presets loaded";
		return;
	}

	char * activeItem = projectm_playlist_item(projectMPlaylistHandle, projectm_playlist_get_position(projectMPlaylistHandle));
	presetName = ofxProjectMPlaylistInternal::formatPresetName(activeItem);
	if (activeItem) {
		projectm_playlist_free_string(activeItem);
	}
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
	std::vector<std::string> previousPlaylistItems;
	if (projectMPlaylistHandle) {
		const auto previousPresetCount = projectm_playlist_size(projectMPlaylistHandle);
		if (previousPresetCount > 0) {
			previousPosition = projectm_playlist_get_position(projectMPlaylistHandle);
			previousPlaylistItems = getPlaylistItems();
			char * currentItem = projectm_playlist_item(projectMPlaylistHandle, previousPosition);
			if (currentItem) {
				currentPresetPath = currentItem;
				projectm_playlist_free_string(currentItem);
			}
		}

		ofxProjectMPlaylistInternal::destroyPlaylistHandle(projectMPlaylistHandle);
	}

	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	if (!projectMPlaylistHandle) {
		setError("projectM_playlist_create failed");
		presetName.clear();
		return;
	}

	connectProjectMCallbacks();
	connectPlaylistCallbacks();
	applyPlaylistParameters();

	const bool restoringExistingPlaylist = !previousPlaylistItems.empty();
	if (restoringExistingPlaylist) {
		auto previousItemsView = makeCStringView(previousPlaylistItems);
		projectm_playlist_add_presets(
			projectMPlaylistHandle,
			ofxProjectMPlaylistInternal::projectMCStringArrayData(previousItemsView),
			static_cast<uint32_t>(previousItemsView.size()),
			true);
	} else {
		const std::string presetPath = ofToDataPath("presets", true);
		projectm_playlist_add_path(projectMPlaylistHandle, presetPath.c_str(), true, false);
	}

	const auto presetCount = projectm_playlist_size(projectMPlaylistHandle);
	if (presetCount == 0) {
		presetName = "No presets loaded";
		setStatus(restoringExistingPlaylist ? "No projectM playlist items loaded." : "No projectM presets loaded.");
		return;
	}
	if (!restoringExistingPlaylist) {
		projectm_playlist_sort(projectMPlaylistHandle, 0, presetCount, SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);
	}

	uint32_t targetPosition =
		restoringExistingPlaylist
			? std::min(previousPosition, presetCount - 1)
			: static_cast<uint32_t>(ofRandom(presetCount));
	// Linear scan to find the current preset by path. Acceptable for typical playlist sizes.
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
	syncPresetNameFromPlaylist();

	if (restoringExistingPlaylist) {
		setStatus("Reloaded projectM playlist.");
		logNotice("ProjectM playlist reloaded.");
		return;
	}

	setStatus(currentPresetPath.empty() ? "Loaded projectM presets." : "Reloaded projectM presets.");
	logNotice(currentPresetPath.empty() ? "Presets loaded." : "Presets reloaded.");
}

void ofxProjectM::presetSwitchRequested(bool hardCut, void * data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(data);
	if (!that || !that->presetSwitchRequestedCallback) {
		return;
	}

	that->presetSwitchRequestedCallback(hardCut);
}

void ofxProjectM::presetSwitched(bool hardCut, unsigned int index, void * data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(data);
	(void)hardCut;
	if (!that || !that->projectMPlaylistHandle) {
		return;
	}

	char * presetPath = projectm_playlist_item(that->projectMPlaylistHandle, index);
	that->presetName = ofxProjectMPlaylistInternal::formatPresetName(presetPath);
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

	that->presetName = std::string("Preset load failed: ") + ofxProjectMPlaylistInternal::formatPresetName(presetFilename);
	if (message) {
		that->setError(message);
	}
}

bool ofxProjectM::playlistPresetLoadRequested(unsigned int index, const char * filename, bool hardCut, void * data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(data);
	if (!that) {
		return false;
	}

	if (that->presetSwitchRequestedCallback) {
		that->presetSwitchRequestedCallback(hardCut);
	}

	if (!that->playlistPresetLoadCallback) {
		// Return false to let the playlist library use its default filesystem loading.
		return false;
	}

	return that->playlistPresetLoadCallback(index, filename ? std::string(filename) : std::string(), hardCut);
}

bool ofxProjectM::loadPresetFile(const std::string & presetPath, bool smoothTransition) {
	clearLastMessages();
	if (!projectMHandle || presetPath.empty()) {
		return false;
	}

	projectm_load_preset_file(projectMHandle, presetPath.c_str(), smoothTransition);
	presetName = ofxProjectMPlaylistInternal::formatPresetName(presetPath.c_str());
	setStatus("Requested preset file load.");
	return true;
}

bool ofxProjectM::loadPresetData(const std::string & presetData, bool smoothTransition) {
	clearLastMessages();
	if (!projectMHandle || presetData.empty()) {
		return false;
	}

	projectm_load_preset_data(projectMHandle, presetData.c_str(), smoothTransition);
	presetName = "Custom preset";
	setStatus("Requested preset data load.");
	return true;
}

void ofxProjectM::setShuffleEnabled(bool enabled) {
	shuffleEnabled = enabled;
	if (projectMPlaylistHandle) {
		projectm_playlist_set_shuffle(projectMPlaylistHandle, shuffleEnabled);
	}
}

bool ofxProjectM::isShuffleEnabled() const {
	return projectMPlaylistHandle ? projectm_playlist_get_shuffle(projectMPlaylistHandle) : shuffleEnabled;
}

void ofxProjectM::setRetryCount(std::uint32_t count) {
	retryCount = count;
	if (projectMPlaylistHandle) {
		projectm_playlist_set_retry_count(projectMPlaylistHandle, retryCount);
	}
}

std::uint32_t ofxProjectM::getRetryCount() const {
	return projectMPlaylistHandle ? projectm_playlist_get_retry_count(projectMPlaylistHandle) : retryCount;
}

void ofxProjectM::clearPlaylist() {
	if (!projectMPlaylistHandle) {
		return;
	}

	projectm_playlist_clear(projectMPlaylistHandle);
	presetName = "No presets loaded";
}

std::vector<std::string> ofxProjectM::getPlaylistItems(std::uint32_t start, std::uint32_t count) const {
	std::vector<std::string> items;
	if (!projectMPlaylistHandle) {
		return items;
	}

	char ** rawItems = projectm_playlist_items(projectMPlaylistHandle, start, count);
	if (!rawItems) {
		return items;
	}

	for (size_t i = 0; rawItems[i] != nullptr; ++i) {
		items.emplace_back(rawItems[i]);
	}
	projectm_playlist_free_string_array(rawItems);
	return items;
}

std::string ofxProjectM::getPlaylistItemPath(std::uint32_t index) const {
	if (!projectMPlaylistHandle) {
		return "";
	}

	return ofxProjectMPlaylistInternal::copyPlaylistString(projectm_playlist_item(projectMPlaylistHandle, index));
}

std::uint32_t ofxProjectM::addPresetPath(const std::string & path, bool recurseSubdirs, bool allowDuplicates) {
	if (!projectMPlaylistHandle || path.empty()) {
		return 0;
	}

	return projectm_playlist_add_path(projectMPlaylistHandle, path.c_str(), recurseSubdirs, allowDuplicates);
}

std::uint32_t ofxProjectM::insertPresetPath(
	const std::string & path,
	std::uint32_t index,
	bool recurseSubdirs,
	bool allowDuplicates) {
	if (!projectMPlaylistHandle || path.empty()) {
		return 0;
	}

	return projectm_playlist_insert_path(projectMPlaylistHandle, path.c_str(), index, recurseSubdirs, allowDuplicates);
}

bool ofxProjectM::addPreset(const std::string & presetPath, bool allowDuplicates) {
	return projectMPlaylistHandle && !presetPath.empty()
		? projectm_playlist_add_preset(projectMPlaylistHandle, presetPath.c_str(), allowDuplicates)
		: false;
}

bool ofxProjectM::insertPreset(const std::string & presetPath, std::uint32_t index, bool allowDuplicates) {
	return projectMPlaylistHandle && !presetPath.empty()
		? projectm_playlist_insert_preset(projectMPlaylistHandle, presetPath.c_str(), index, allowDuplicates)
		: false;
}

std::uint32_t ofxProjectM::addPresets(const std::vector<std::string> & presetPaths, bool allowDuplicates) {
	if (!projectMPlaylistHandle || presetPaths.empty()) {
		return 0;
	}

	auto view = makeCStringView(presetPaths);
	return projectm_playlist_add_presets(
		projectMPlaylistHandle,
		ofxProjectMPlaylistInternal::projectMCStringArrayData(view),
		static_cast<uint32_t>(view.size()),
		allowDuplicates);
}

std::uint32_t ofxProjectM::insertPresets(
	const std::vector<std::string> & presetPaths,
	std::uint32_t index,
	bool allowDuplicates) {
	if (!projectMPlaylistHandle || presetPaths.empty()) {
		return 0;
	}

	auto view = makeCStringView(presetPaths);
	return projectm_playlist_insert_presets(
		projectMPlaylistHandle,
		ofxProjectMPlaylistInternal::projectMCStringArrayData(view),
		static_cast<uint32_t>(view.size()),
		index,
		allowDuplicates);
}

bool ofxProjectM::removePreset(std::uint32_t index) {
	return projectMPlaylistHandle ? projectm_playlist_remove_preset(projectMPlaylistHandle, index) : false;
}

std::uint32_t ofxProjectM::removePresets(std::uint32_t index, std::uint32_t count) {
	return projectMPlaylistHandle ? projectm_playlist_remove_presets(projectMPlaylistHandle, index, count) : 0;
}

void ofxProjectM::sortPlaylist(
	std::uint32_t startIndex,
	std::uint32_t count,
	ofxProjectMPlaylistSortPredicate predicate,
	ofxProjectMPlaylistSortOrder order) {
	if (!projectMPlaylistHandle) {
		return;
	}

	projectm_playlist_sort(
		projectMPlaylistHandle,
		startIndex,
		count,
		static_cast<projectm_playlist_sort_predicate>(predicate),
		static_cast<projectm_playlist_sort_order>(order));
}

void ofxProjectM::setPlaylistFilter(const std::vector<std::string> & filters) {
	playlistFilter = filters;
	if (!projectMPlaylistHandle) {
		return;
	}

	if (playlistFilter.empty()) {
		projectm_playlist_set_filter(projectMPlaylistHandle, nullptr, 0);
		return;
	}

	auto view = makeCStringView(playlistFilter);
	projectm_playlist_set_filter(
		projectMPlaylistHandle,
		ofxProjectMPlaylistInternal::projectMCStringArrayData(view),
		view.size());
}

const std::vector<std::string> & ofxProjectM::getPlaylistFilter() const {
	return playlistFilter;
}

std::size_t ofxProjectM::applyPlaylistFilter() {
	return projectMPlaylistHandle ? projectm_playlist_apply_filter(projectMPlaylistHandle) : 0;
}

int ofxProjectM::playLastPreset(bool hardCut) const {
	if (!projectMPlaylistHandle) {
		return -1;
	}

	return static_cast<int>(projectm_playlist_play_last(projectMPlaylistHandle, hardCut));
}

void ofxProjectM::setPresetSwitchRequestedCallback(const std::function<void(bool)> & callback) {
	presetSwitchRequestedCallback = callback;
	if (projectMPlaylistHandle) {
		projectm_playlist_set_preset_load_event_callback(
			projectMPlaylistHandle,
			(presetSwitchRequestedCallback || playlistPresetLoadCallback) ? playlistPresetLoadRequested : nullptr,
			this);
		return;
	}

	if (projectMHandle) {
		projectm_set_preset_switch_requested_event_callback(
			projectMHandle,
			presetSwitchRequestedCallback ? presetSwitchRequested : nullptr,
			this);
	}
}

void ofxProjectM::clearPresetSwitchRequestedCallback() {
	setPresetSwitchRequestedCallback({});
}

void ofxProjectM::setPlaylistPresetLoadCallback(
	const std::function<bool(unsigned int, const std::string &, bool)> & callback) {
	playlistPresetLoadCallback = callback;
	if (projectMPlaylistHandle) {
		projectm_playlist_set_preset_load_event_callback(
			projectMPlaylistHandle,
			(presetSwitchRequestedCallback || playlistPresetLoadCallback) ? playlistPresetLoadRequested : nullptr,
			this);
	}
}

void ofxProjectM::clearPlaylistPresetLoadCallback() {
	setPlaylistPresetLoadCallback({});
}

void ofxProjectM::previousPreset(bool hardCut) const {
	if (projectMPlaylistHandle) {
		projectm_playlist_play_previous(projectMPlaylistHandle, hardCut);
	}
}

void ofxProjectM::nextPreset(bool hardCut) const {
	if (projectMPlaylistHandle) {
		projectm_playlist_play_next(projectMPlaylistHandle, hardCut);
	}
}

void ofxProjectM::randomPreset() const {
	if (!projectMPlaylistHandle) {
		return;
	}

	const auto presetCount = projectm_playlist_size(projectMPlaylistHandle);
	if (presetCount > 0) {
		projectm_playlist_set_position(projectMPlaylistHandle, static_cast<uint32_t>(ofRandom(presetCount)), true);
	}
}

bool ofxProjectM::restartPreset(bool hardCut) const {
	const int presetIndex = getPresetIndex();
	if (presetIndex < 0) {
		return false;
	}

	return setPresetIndex(presetIndex, hardCut);
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
