#ifndef LABEL_TOOLS_VIDEO_YAML_LOADER_H
#define LABEL_TOOLS_VIDEO_YAML_LOADER_H

#include <iostream>
#include <map>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "common.h"

class YamlLoader {
public:
	YamlLoader(std::string config_file, std::string appname);
	
	std::map<std::string, std::string> getVideoFiles() {
		return video_files_;
	}
	
	MyLabel getClassNames() {
		return classnames_;
	}
	
	std::string getLabeledImagesDir() {
		return labeled_images_dir_;
	}
	
private:
	
	void createTargetDir(std::string dir, std::string subdir = "");
	void initClassNames(YAML::Node data, std::string filename);
	void findVideoFiles(YAML::Node data);

	std::string appname_;
	YAML::Node node_;
	std::string output_dir_, patched_dir_, labeled_images_dir_;
	
	// Will be shared with others
	MyLabel classnames_;
	std::map<std::string, std::string> video_files_;
};
#endif
