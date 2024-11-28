#include "ofxProjectM.h"
#include <iostream>

ofxProjectM::~ofxProjectM(){
	projectm_destroy(projectMHandle);
	projectm_playlist_destroy(projectMPlaylistHandle);
}

void ofxProjectM::load(){
	windowWidth = 800;
	windowHeight = 600;
	std::cout << projectm_get_version_string() << std::endl;

	projectMHandle = projectm_create();
	projectm_set_window_size(projectMHandle, windowWidth, windowHeight);
	projectm_set_mesh_size(projectMHandle, 48, 32);
	projectm_set_aspect_correction(projectMHandle, true);
	projectm_set_fps(projectMHandle, 60);
	projectm_set_soft_cut_duration(projectMHandle, 5);
	projectm_set_preset_locked(projectMHandle, false);
	projectm_set_preset_duration(projectMHandle, 30.0);
	std::vector<const char*> textures = { "data/textures" };
	projectm_set_texture_search_paths(projectMHandle, textures.data(), 1);
	
	projectMPlaylistHandle = projectm_playlist_create(projectMHandle);
	projectm_playlist_add_path(projectMPlaylistHandle, "data/presets", true, false);
	projectm_playlist_set_shuffle(projectMPlaylistHandle, true);
	projectm_playlist_sort(projectMPlaylistHandle, 0, projectm_playlist_size(projectMPlaylistHandle), SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);
	projectm_playlist_play_next(projectMPlaylistHandle, true);

	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
}

void ofxProjectM::draw(int x, int y) {
	fbo.begin();
	projectm_opengl_render_frame_fbo(projectMHandle, fbo.getId());
	fbo.end();
	fbo.draw(x, y);
}

void ofxProjectM::draw(int x, int y, int a, int b) {
	fbo.begin();
	projectm_opengl_render_frame_fbo(projectMHandle, fbo.getId());
	fbo.end();
	fbo.draw(x, y, a, b);
}

void ofxProjectM::nextPreset() {
	projectm_playlist_play_next(projectMPlaylistHandle, true);
}

void ofxProjectM::randomPreset() {
	projectm_playlist_set_position(projectMPlaylistHandle, ofRandom(0, projectm_playlist_size(projectMPlaylistHandle) - 1), false);
}
char* ofxProjectM::getPresetName() {
	return projectm_playlist_item(projectMPlaylistHandle, projectm_playlist_get_position(projectMPlaylistHandle));
}

ofTexture ofxProjectM::getTexture() {
	return fbo.getTexture();
}

int ofxProjectM::getMaxSamples() {
	return projectm_pcm_get_max_samples();
}

void ofxProjectM::audio(float* buffer) {
	projectm_pcm_add_float(projectMHandle, buffer, 512, PROJECTM_STEREO);
}
