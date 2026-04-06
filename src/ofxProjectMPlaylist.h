#pragma once

#include "projectM-4/playlist.h"

#include <string>
#include <vector>

namespace ofxProjectMPlaylistInternal {
std::string formatPresetName(const char * presetPath);
std::string copyPlaylistString(char * value);
const char ** projectMCStringArrayData(std::vector<const char *> & values);
void destroyPlaylistHandle(projectm_playlist_handle & playlistHandle);
}
