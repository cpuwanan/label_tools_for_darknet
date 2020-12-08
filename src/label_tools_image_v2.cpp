
#include <opencv2/opencv.hpp>

#include <fstream>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <map>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

#include "utils.h"
#include "label_tools_image_v2/common.h"
#include "label_tools_image_v2/my_tools.h"

class FileManager {
public:
	FileManager() {
		
	}
	
	~FileManager() {}
	
	bool loadParams(std::string config_file) {
		std::ifstream reader;
		reader.open(config_file);
		if (!reader.is_open()) {
			return false;
		}
		reader.close();
		
		YAML::Node node = YAML::LoadFile(config_file);
		std::string param = "input_image_dir";
		if (node[param]) {
			input_image_dir_ = node[param].as<std::string>();
		} else {
			std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Param '%s' was not defined", param.c_str())) << std::endl;
			return false;
		}
		
		param = "output_image_dir";
		if (node[param]) {
			output_dir_ = node[param].as<std::string>();
		} else {
			std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Param '%s' was not defined", param.c_str())) << std::endl;
			return false;
		}
		
		param = "object_classnames";
		annotations_.clear();
		if (node[param]) {
			auto data = node[param];
			if (data.size() > 0) {
				for (int i=0; i<(int)data.size(); i++) {
					MyLabelInfo info;
					info.id = i;
					info.color = cv::Scalar(rand() % 200, rand()%200, rand()%200);
					annotations_.insert(std::pair<std::string, MyLabelInfo>( data[i].as<std::string>(), info));
				}
			} else {
				std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Data of '%s' cannot be empty", param.c_str())) << std::endl;
				return false;
			}
		} else {
			std::cout << utils::colorText(TextType::DANGER_B, cv::format("Param '%s' was not defined", param.c_str())) << std::endl;
			return false;
		}
		
		if (annotations_.size() == 0) {
			std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Cannot init data for '%s'", param.c_str())) << std::endl;
			return false;
		} else {
			std::stringstream ss;
			for (auto it = annotations_.begin(); it != annotations_.end(); it++) {
				ss << it->first << " ";
			}
			std::cout << "[OK] Successfully init " << int(annotations_.size()) << " objects: " << utils::colorText(SUCCESS_B, ss.str()) << std::endl;
		}
		
		this->createOutputDirectories(output_dir_);
		this->findImageFilenames(input_image_dir_);
		
		if (image_filenames_.size() == 0) {
			std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Cannot find any image in '%s'", input_image_dir_.c_str())) << std::endl;
			return false;
		}
		return true;
	}
	
	MyLabel getAnnotations() { return annotations_; }
	std::string getInputDir() { return input_image_dir_; }
	std::string getOutputDir() { return output_dir_; }
	std::vector<std::string> getImageFilenames() { return image_filenames_; }
	
private:
	
	void findImageFilenames(std::string dir) {
		namespace fs = boost::filesystem;
		image_filenames_.clear();
		fs::path path(dir);
		if (!fs::exists(path)) {
			std::cout << "[FAILED] " << utils::colorText(TextType::DANGER_B, cv::format("Path '%s' does not exist", dir.c_str())) << std::endl;
			return;
		}
		
		std::vector<std::string> target_subfix;
		target_subfix.push_back(".png");
		target_subfix.push_back(".jpg");
		target_subfix.push_back(".jpeg");
		
		std::stringstream ss;
		fs::directory_iterator end_itr;
		for (fs::directory_iterator itr(path); itr != end_itr; ++itr) {
			if (fs::is_regular_file(itr->path())) {
				std::string filename = itr->path().string();
				bool is_valid = false;
				for (int i=0; i<target_subfix.size() && !is_valid; i++) {
					std::size_t found = filename.find(target_subfix[i]);
					if (found != std::string::npos) {
						is_valid = true;
					}
				}
				if (is_valid) {
					int length = filename.size() - dir.size() - 1;
					filename = filename.substr(dir.size() + 1, length);
					ss << " (" << int(image_filenames_.size()) + 1 << ") " << filename << std::endl;
					image_filenames_.push_back(filename);
				}
			}
		}
		
		if (image_filenames_.size() > 0) {
			std::cout << "Found: " << utils::colorText(SUCCESS_B, cv::format("%d image filenames", int(image_filenames_.size()))) << "\n" << ss.str() << std::endl;
		}
	}
	
	void createOutputDirectories(std::string dir) {
		namespace fs = boost::filesystem;
		
		bool is_done = false;
		std::string root_path = dir;
		std::vector<std::string> target_paths;
		while (!is_done) {
			std::cout << "Target: " << root_path << std::endl;
			fs::path path(root_path);
			if (!fs::exists(path)) {
				target_paths.push_back(root_path);
				std::cout << " -- " << utils::colorText(WARNING_B, "Does not exist") << std::endl;
				std::string input = root_path;
				if (!this->getRootPath(input, root_path)) {
					is_done = true;
				}
				std::cout << " -- Root: " << root_path << std::endl;
			} else {
				is_done = true;
			}
		}
		
		std::cout << "Create directories for output" << std::endl;
		std::reverse(target_paths.begin(), target_paths.end());
		for (std::vector<std::string>::iterator it = target_paths.begin(); it != target_paths.end(); it++) {
			fs::create_directories(*it);
			std::cout << " -- Created: " << utils::colorText(SUCCESS, (*it).c_str()) << std::endl;
		}
	}
	
	bool getRootPath(std::string dir, std::string &root) {
		int index = -1;
		for (int i=dir.size() - 1; i>=0 && index == -1; i--) {
			if (dir[i] == '/') { index = i; }
		}
		if (index != -1) {
			root = dir.substr(0, index);
			return true;
		}
		return false;
	}
	
	std::string input_image_dir_, output_dir_;
	MyLabel annotations_;
	std::vector<std::string> image_filenames_;
};

int main(int argc, char **argv) {
	
	std::string config_file = "../config/label_tools_image_v2.yaml";
	if (argc > 1) {
		config_file = argv[1];
	}
	
	std::cout << "Reading config from " << utils::colorText(TextType::SUCCESS_B, config_file) << std::endl;
	
	FileManager manager;
	if (!manager.loadParams(config_file)) {
		std::cout << utils::colorText(TextType::DANGER_B, "Failed reading config") << std::endl;
		return EXIT_FAILURE;
	}
	
	MyTools tools("viewer", "settings");
	tools.setClassNames(manager.getAnnotations());
	tools.setImageFilenames(
		manager.getImageFilenames(), 
		manager.getInputDir(), 
		manager.getOutputDir()
	);
	tools.run();
	
	return 0;
}
