#include "ofxProjectM.h"

ofxProjectM::~ofxProjectM() {
	projectm_destroy(projectMHandle);
	projectm_playlist_destroy(projectMPlaylistHandle);
}

void ofxProjectM::load() {
	ofSetRandomSeed(ofGetSystemTimeMillis());
	windowWidth = 1024;
	windowHeight = 1024;
	std::cout << "projectM version: " << projectm_get_version_string() << std::endl;
	// std::cout << "projectM max samples: " << projectm_pcm_get_max_samples() << std::endl;

	projectMHandle = projectm_create();
	projectm_set_window_size(projectMHandle, windowWidth, windowHeight);
	projectm_set_mesh_size(projectMHandle, 32, 32);
	projectm_set_aspect_correction(projectMHandle, true);
	projectm_set_fps(projectMHandle, 60);
	projectm_set_beat_sensitivity(projectMHandle, 0.5);
	projectm_set_hard_cut_enabled(projectMHandle, true);
	projectm_set_hard_cut_duration(projectMHandle, 5.0);
	projectm_set_hard_cut_sensitivity(projectMHandle, 0.5);
	projectm_set_soft_cut_duration(projectMHandle, 5.0);
	projectm_set_preset_locked(projectMHandle, false);
	projectm_set_preset_duration(projectMHandle, 30.0);
	std::vector<const char*> textures = { "data/textures" };
	projectm_set_texture_search_paths(projectMHandle, textures.data(), 1);

	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	projectm_playlist_add_path(projectMPlaylistHandle, "data/presets", true, false);
	projectm_playlist_set_retry_count(projectMPlaylistHandle, 0);
	projectm_playlist_set_shuffle(projectMPlaylistHandle, true);
	projectm_playlist_sort(projectMPlaylistHandle, 0, projectm_playlist_size(projectMPlaylistHandle), SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);
	projectm_playlist_set_position(projectMPlaylistHandle, ofRandom(0, projectm_playlist_size(projectMPlaylistHandle) - 1), true);
	presetName = projectm_playlist_item(projectMPlaylistHandle, projectm_playlist_get_position(projectMPlaylistHandle));
	
	projectm_playlist_set_preset_switched_event_callback(projectMPlaylistHandle, presetSwitched, this);
	projectm_playlist_set_preset_switch_failed_event_callback(projectMPlaylistHandle, presetSwitchFailed, this);
	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
}

void ofxProjectM::presetSwitched(bool hardCut, unsigned int index, void* data) {
	ofxProjectM* that = static_cast<ofxProjectM*>(data);
	that->presetName = projectm_playlist_item(that->projectMPlaylistHandle, index);
}

void ofxProjectM::presetSwitchFailed(const char* presetFilename, const char* message, void* data) {
	ofxProjectM* that = static_cast<ofxProjectM*>(data);
	std::cout << message << std::endl;
}

void ofxProjectM::setWindowSize(int x, int y) {
	windowWidth = x;
	windowHeight = y;
	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
	projectm_set_window_size(projectMHandle, windowWidth, windowHeight);
}

void ofxProjectM::setMeshSize(int x, int y) const {
	projectm_set_mesh_size(projectMHandle, x, y);
}

void ofxProjectM::setPresetDuration(double duration) const {
	projectm_set_preset_duration(projectMHandle, duration);
}

void ofxProjectM::update() {
	fbo.bind();
	projectm_opengl_render_frame_fbo(projectMHandle, fbo.getId());
	fbo.unbind();
}

void ofxProjectM::draw(int x, int y) {
	fbo.getTexture().draw(x, y);
}

void ofxProjectM::draw(int x, int y, int a, int b) {
	fbo.getTexture().draw(x, y, a, b);
}

void ofxProjectM::bind() {
	fbo.getTexture().bind();
}

void ofxProjectM::unbind() {
	fbo.getTexture().unbind();
}

void ofxProjectM::nextPreset() const {
	projectm_playlist_play_next(projectMPlaylistHandle, true);
}

void ofxProjectM::randomPreset() const {
	projectm_playlist_set_position(projectMPlaylistHandle, ofRandom(0, projectm_playlist_size(projectMPlaylistHandle) - 1), true);
}

std::string ofxProjectM::getPresetName() {
	return presetName;
}

int ofxProjectM::getMaxSamples() {
	return projectm_pcm_get_max_samples();
}

void ofxProjectM::audio(float* buffer, int bufferSize, int channels) const {
	projectm_pcm_add_float(projectMHandle, buffer, bufferSize, (projectm_channels)channels);
}
