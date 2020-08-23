#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <map>

#include <yaml-cpp/yaml.h>
#include <boost/filesystem.hpp>

#include "utils.h"
#include "label_tools_video/yaml_loader.h"
#include "label_tools_video/my_tools.h"

static void showUsage(std::string name) {
	std::stringstream ss;
	ss << "Usage: " << name << " <options> <value>"
		<< "\nOptions:"
		<< "\n  -h, --help\tShow this help message"
		<< "\n  -c, --config\tConfig about the training"
		<< "\n  -d, --dest\tSpecifiy target saved folder";
	std::cout << utils::colorText(TextType::INFO, ss.str()) << std::endl;
}

void checkInput(int argc, char **argv, int &i, std::string key, std::string &value) {
	if (i+1 < argc) {
		value = argv[i+1];
		i = i+1;
	} else {
		std::cerr << utils::colorText(TextType::DANGER_B, "'" + key + "' option requires one argument") << std::endl;
	}
}

int main(int argc, char **argv) {
	
	// Create a folder to store results
	std::string appname = argv[0];
	std::size_t found = appname.find("/");
	if (found != std::string::npos) {
		appname = appname.substr(int(found) + 1, appname.size() - int(found));
		std::cout << "\n ** App name: " << utils::colorText(TextType::SUCCESS, appname) << std::endl;
	}
	
	if (argc < 2) {
		showUsage(argv[0]);
		return 1;
	}
	
	std::vector<std::string> inputs;
	std::string target_dir(""), config_file("");
	
	for (int i=1; i<argc; i++) {
		std::string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			showUsage(argv[0]);
			return 1;
		} else if (arg == "-d" || arg == "--dest") {
			checkInput(argc, argv, i, "--dest", target_dir);
		} else if (arg == "-c" || arg == "--config") {
			checkInput(argc, argv, i, "--config", config_file);
		}
	}
	
	if (config_file == "") {
		std::cout << utils::colorText(TextType::DANGER_B, "Invalid config file") << std::endl;
		return -1;
	}
	
	std::cout << " ** Set target cconfig file: " << utils::colorText(TextType::SUCCESS, config_file) << std::endl;
	
	YamlLoader loader(config_file, appname);
	
	MyTools label_tools(appname, "settings");
	label_tools.setClassNames(loader.getClassNames());
	label_tools.setVideoFiles(loader.getVideoFiles());
	label_tools.setSavedImageType(".png");
	label_tools.setLabeledImagesDir(loader.getLabeledImagesDir());
	label_tools.loop();
	
	return 0;
}
