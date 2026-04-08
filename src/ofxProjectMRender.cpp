#include "ofxProjectM.h"
#include "ofxProjectMPlaylist.h"
#include "ofxProjectMRender.h"

#include <algorithm>

int ofxProjectMRenderInternal::sanitizeChannelCount(int channels) {
	return std::max(1, channels);
}

void ofxProjectMRenderInternal::clearTextureLoadData(projectm_texture_load_data * data) {
	if (!data) {
		return;
	}

	data->data = nullptr;
	data->width = 0;
	data->height = 0;
	data->texture_id = 0;
}

void ofxProjectMRenderInternal::populateTextureLoadData(const ofTexture & texture, projectm_texture_load_data * data) {
	if (!data) {
		return;
	}

	data->texture_id = texture.getTextureData().textureID;
	data->width = texture.getWidth();
	data->height = texture.getHeight();
}

void ofxProjectMRenderInternal::clearAllocatedFbo(ofFbo & fbo) {
	if (!fbo.isAllocated()) {
		return;
	}

	fbo.begin();
	ofClear(0, 0, 0, 0);
	fbo.end();
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

void ofxProjectM::setTextureSearchPaths(const std::vector<std::string> & searchPaths) {
	textureSearchPaths.clear();
	for (const auto & path : searchPaths) {
		if (!path.empty()) {
			textureSearchPaths.push_back(path);
		}
	}

	if (projectMHandle) {
		auto paths = makeCStringView(textureSearchPaths);
		projectm_set_texture_search_paths(
			projectMHandle,
			ofxProjectMPlaylistInternal::projectMCStringArrayData(paths),
			paths.size());
	}
}

const std::vector<std::string> & ofxProjectM::getTextureSearchPaths() const {
	return textureSearchPaths;
}

void ofxProjectM::writeDebugImageOnNextFrame(const std::string & outputPath) const {
	if (projectMHandle && !outputPath.empty()) {
		projectm_write_debug_image_on_next_frame(projectMHandle, outputPath.c_str());
	}
}

void ofxProjectM::textureLoadEvent(const char * texture_name, projectm_texture_load_data * data, void * user_data) {
	ofxProjectM * that = static_cast<ofxProjectM *>(user_data);
	(void)texture_name;
	if (!that || !data) {
		return;
	}

	if (!that->texture.isAllocated()) {
		ofxProjectMRenderInternal::clearTextureLoadData(data);
		return;
	}

	ofxProjectMRenderInternal::populateTextureLoadData(that->texture, data);
}

void ofxProjectM::setWindowSize(int x, int y) {
	const int clampedWidth = std::max(1, x);
	const int clampedHeight = std::max(1, y);
	if (clampedWidth == windowWidth && clampedHeight == windowHeight && fbo.isAllocated()) {
		return;
	}

	windowWidth = clampedWidth;
	windowHeight = clampedHeight;
	fbo.allocate(windowWidth, windowHeight, GL_RGBA);
	ofxProjectMRenderInternal::clearAllocatedFbo(fbo);
	if (projectMHandle) {
		projectm_set_window_size(projectMHandle, static_cast<size_t>(windowWidth), static_cast<size_t>(windowHeight));
	}
}

void ofxProjectM::getWindowSize(int & x, int & y) const {
	if (projectMHandle) {
		size_t width = 0;
		size_t height = 0;
		projectm_get_window_size(projectMHandle, &width, &height);
		x = static_cast<int>(width);
		y = static_cast<int>(height);
		return;
	}

	x = windowWidth;
	y = windowHeight;
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
	// FBO not yet allocated (before init()): fall back to the external texture
	// supplied via setTexture(). This may be an empty/invalid texture.
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

int ofxProjectM::getMaxSamples() const {
	return projectm_pcm_get_max_samples();
}

void ofxProjectM::audio(const float * buffer, int bufferSize, int channels) const {
	if (projectMHandle && buffer && bufferSize > 0 && channels > 0) {
		projectm_pcm_add_float(
			projectMHandle,
			buffer,
			bufferSize,
			static_cast<projectm_channels>(ofxProjectMRenderInternal::sanitizeChannelCount(channels)));
	}
}

void ofxProjectM::audio(const int16_t * buffer, int bufferSize, int channels) const {
	if (projectMHandle && buffer && bufferSize > 0 && channels > 0) {
		projectm_pcm_add_int16(
			projectMHandle,
			buffer,
			bufferSize,
			static_cast<projectm_channels>(ofxProjectMRenderInternal::sanitizeChannelCount(channels)));
	}
}

void ofxProjectM::audio(const uint8_t * buffer, int bufferSize, int channels) const {
	if (projectMHandle && buffer && bufferSize > 0 && channels > 0) {
		projectm_pcm_add_uint8(
			projectMHandle,
			buffer,
			bufferSize,
			static_cast<projectm_channels>(ofxProjectMRenderInternal::sanitizeChannelCount(channels)));
	}
}

void ofxProjectM::audio(const ofSoundBuffer & soundBuffer) const {
	const std::vector<float> & data = soundBuffer.getBuffer();
	if (!data.empty()) {
		audio(data.data(),
			static_cast<int>(soundBuffer.getNumFrames()),
			static_cast<int>(soundBuffer.getNumChannels()));
	}
}

void ofxProjectM::touch(float x, float y, int pressure, ofxProjectMTouchType type) const {
	if (projectMHandle) {
		projectm_touch(projectMHandle, x, y, pressure, static_cast<projectm_touch_type>(type));
	}
}

void ofxProjectM::touchDrag(float x, float y, int pressure) const {
	if (projectMHandle) {
		projectm_touch_drag(projectMHandle, x, y, pressure);
	}
}

void ofxProjectM::touchDestroy(float x, float y) const {
	if (projectMHandle) {
		projectm_touch_destroy(projectMHandle, x, y);
	}
}

void ofxProjectM::clearTouches() const {
	if (projectMHandle) {
		projectm_touch_destroy_all(projectMHandle);
	}
}

uint32_t ofxProjectM::createSprite(const std::string & type, const std::string & code) const {
	if (!projectMHandle || type.empty() || code.empty()) {
		return 0;
	}

	return projectm_sprite_create(projectMHandle, type.c_str(), code.c_str());
}

void ofxProjectM::destroySprite(uint32_t spriteId) const {
	if (projectMHandle && spriteId != 0) {
		projectm_sprite_destroy(projectMHandle, spriteId);
	}
}

void ofxProjectM::clearSprites() const {
	if (projectMHandle) {
		projectm_sprite_destroy_all(projectMHandle);
	}
}

uint32_t ofxProjectM::getSpriteCount() const {
	return projectMHandle ? projectm_sprite_get_sprite_count(projectMHandle) : 0;
}

std::vector<uint32_t> ofxProjectM::getSpriteIds() const {
	const uint32_t count = getSpriteCount();
	std::vector<uint32_t> spriteIds(count, 0);
	if (projectMHandle && count > 0) {
		projectm_sprite_get_sprite_ids(projectMHandle, spriteIds.data());
	}
	return spriteIds;
}

void ofxProjectM::setMaxSprites(uint32_t maxSprites) const {
	if (projectMHandle) {
		projectm_sprite_set_max_sprites(projectMHandle, maxSprites);
	}
}

uint32_t ofxProjectM::getMaxSprites() const {
	return projectMHandle ? projectm_sprite_get_max_sprites(projectMHandle) : 0;
}

