#pragma once

#include "projectM-4/projectM.h"

class ofTexture;
class ofFbo;

namespace ofxProjectMRenderInternal {
int sanitizeChannelCount(int channels);
void clearAllocatedFbo(ofFbo & fbo);
void clearTextureLoadData(projectm_texture_load_data * data);
void populateTextureLoadData(const ofTexture & texture, projectm_texture_load_data * data);
}
