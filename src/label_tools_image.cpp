#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <ctime>
#include <fstream>
#include <map>

#include <boost/filesystem.hpp>
#include <yaml-cpp/yaml.h>

#include "utils.h"
#include "label_tools_image/my_tools.h"

namespace my_utils {
	std::string getNameFromFile(std::string filename, char startBit, int endBitPos) {
		std::string name("");
    int startIndex = -1;
    for (int i=endBitPos; i>=0 && startIndex == -1; i--) {
      if (filename[i] == startBit) {
        startIndex = i;
      }
    }
    if (startIndex != -1) {
      int length = endBitPos - startIndex - 1;
      name = filename.substr(startIndex + 1, length);
    }
		return name;
	}
};

class FileManager {
public:
  FileManager(std::string config_file) 
    : isOk(false)
  {
    YAML::Node node = YAML::LoadFile(config_file);
    
    // Step 1: Set output information: to create root_dir for annotation file
    if (node["output"]) {
      if (node["output"]["root_dir"]) {
        rootDir = node["output"]["root_dir"].as<std::string>();
        bool isAvailable = boost::filesystem::exists(rootDir);
        if (!isAvailable) {
          std::cout << utils::colorText(TextType::DANGER_B, "Invalid dir: " + rootDir) << std::endl;
          isOk = false;
          return;
        }
        
        if (node["output"]["image_folder"]) {
          outputImageDir = rootDir + "/" + node["output"]["image_folder"].as<std::string>();
          bool isAvailable = boost::filesystem::exists(outputImageDir);
          if (!isAvailable) {
            std::cout << utils::colorText(TextType::SUCCESS, "Create a dir: " + outputImageDir) << std::endl;
            boost::filesystem::create_directories(outputImageDir);
          }
        }
      }
      
      if (node["output"]["image_subfix"]) {
        imageSubfix = node["output"]["image_subfix"].as<std::string>();
      }
    }
    
    // Step 2: registrer class names
    isOk = this->createAnnotations(node);
    if (!isOk) {
      return;
    }
    
    // Set input information
    if (node["input"]) {
      if (node["input"]["image_dir"]) {
        inputImageDir = node["input"]["image_dir"].as<std::string>();
        bool isAvailable = boost::filesystem::exists(inputImageDir);
        if (!isAvailable) {
          std::cout << utils::colorText(TextType::DANGER_B, "Invalid dir: " + inputImageDir) << std::endl;
          isOk = false;
          return;
        }
        
        std::vector<std::string> labelFiles;
        for (auto &p : boost::filesystem::recursive_directory_iterator(inputImageDir)) {
          std::string filename = p.path().c_str();
          std::size_t found1 = filename.find(".txt");
          if (found1 != std::string::npos) {
            labelFiles.push_back(filename);
          }
        }
        
        int index = 0;
        for (auto &p : boost::filesystem::recursive_directory_iterator(inputImageDir)) {
          std::string filename = p.path().c_str();
          std::size_t found1 = filename.find(".png");
          std::size_t found2 = filename.find(".jpg");
          if (found1 != std::string::npos || found2 != std::string::npos) {
            int endBitPos = std::max(int(found1), int(found2));
            std::string id = my_utils::getNameFromFile(filename, '/', endBitPos);
            cv::Mat image = cv::imread(filename, 1);
            MyImage labeledInfo(image, id, index);
            std::cout << " -- ID: " << id << std::endl;
            std::vector<std::string>::iterator it;
            std::string matchedLabelFile("");
            for (it = labelFiles.begin(); it != labelFiles.end() && matchedLabelFile.empty(); it++) {
              std::size_t found3 = (*it).find(id);
              if (found3 != std::string::npos) {
                matchedLabelFile = (*it);
              }
            }
            if (!matchedLabelFile.empty()) {
              labeledInfo.addObjectsFromLabelFile(matchedLabelFile, annotations);
            }
            images.push_back(labeledInfo);
            index++;
          }
        }
        std::cout << " ** Loaded " << utils::colorText(TextType::SUCCESS, cv::format("%d images successfully", int(images.size()))) << std::endl;
      }
    }
    
    if (int(images.size() == 0)) {
      std::cout << " ** " << utils::colorText(TextType::DANGER, "ERROR: No available images for training") << std::endl;
      return;
    }
    
    isOk = true;
  }
  
  bool createAnnotations(YAML::Node &node) {
    
    annotations.clear();
    classnames.clear();
    
    // Create annotation file
    if (node["training"] && rootDir != "") {
      std::string annotationFile = "annotation.names";
      if (node["training"]["annotation_filename"]) {
          annotationFile = node["training"]["annotation_filename"].as<std::string>();
      }
      
      if (node["training"]["class_names"]) {
        auto data = node["training"]["class_names"];
        if ((int)data.size() > 0) {
          // Create a file for writing
          std::ofstream file;
          file.open(rootDir + "/" + annotationFile);
          for (int i=0; i<data.size(); i++) {
            std::string obj = data[i].as<std::string>();
            file << obj << "\n";
            annotations.insert(std::pair<std::string, int>(obj, i));
            MyLabelInfo info;
            info.id = i;
            info.color = cv::Scalar(rand() % 200, rand() % 200, rand() % 200);
            classnames.insert(std::pair<std::string, MyLabelInfo>(obj, info));
          }
          file.close();
        }
      }
    }
    
    if (annotations.size() == 0) {
      std::cout << " ** " << utils::colorText(TextType::DANGER, "Invalid annotations") << std::endl;
      return false;
    }
    std::cout << " ** Created annotation size: " << int(annotations.size()) << std::endl;
    
    return true;
  }
  
  bool save(std::vector<MyImage> &images) {
    if ((int)images.size() > 0) {
      std::cout << "\n " << utils::colorText(TextType::SUCCESS_B, cv::format("Saving all %d labeled images", int(images.size()))) << std::endl;
      std::vector<MyImage>::iterator it;

      for (it = images.begin(); it != images.end(); it++) {
        
        auto objects = it->getObjects();
        if (objects.size() > 0) {
          std::string prefix = cv::format("%s/%s", outputImageDir.c_str(), it->getImageId().c_str());
          std::string imageFile = prefix + imageSubfix;
          std::string labelBoxFile = prefix + ".txt";
          std::ofstream myfile;
          myfile.open(labelBoxFile);	
          cv::imwrite(imageFile, it->getImage());
          std::cout << "[" << it->getImageId() << "] Img: " << utils::colorText(TextType::SUCCESS_B, imageFile) << std::endl;
          std::map<std::string, LabeledObject>::iterator it3;
          for (it3 = objects.begin(); it3 != objects.end(); it3++) {
            myfile << it3->second.getLabelInfoStr() << "\n";
          }
          myfile.close();
        }
      }
    } else {
      std::cout << " ** " << utils::colorText(TextType::DANGER_B, "No available labeled image") << std::endl;
    }
  }
    
  // Public parameters
  bool isOk;
  std::map<std::string, int> annotations;
  std::string rootDir;
  std::string outputImageDir, inputImageDir;
  std::string imageSubfix = ".png";
  std::vector<MyImage> images;
  MyLabel classnames;
  
private:
  
};

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
	
  FileManager manager(config_file);
  MyTools label_tools(appname);
  label_tools.setClassNames(manager.classnames);

  std::vector<MyImage> images = manager.images;
  label_tools.loop(images);
  bool res = manager.save(images);

	return 0;
}
