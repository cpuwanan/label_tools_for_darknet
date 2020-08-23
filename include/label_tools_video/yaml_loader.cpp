#include "yaml_loader.h"
#include "utils.h"
#include <boost/filesystem.hpp>
#include <fstream>

YamlLoader::YamlLoader(std::string config_file, std::string appname)
	: appname_(appname)
	, patched_dir_("")
{
	node_ = YAML::LoadFile(config_file);
	
	if (node_["output"]) {
		if (node_["output"]["root_dir"]) {
			bool save_patched_images = false;
			if (node_["deep_learning"]) {
				if (node_["deep_learning"]["save_patched_images"]) {
					save_patched_images = node_["deep_learning"]["save_patched_images"].as<bool>();
				}
			}
			output_dir_ = node_["output"]["root_dir"].as<std::string>();
			this->createTargetDir(output_dir_, "patched_images");
		}
		
		if (node_["output"]["labeled_images_dir"]) {
			labeled_images_dir_ = node_["output"]["labeled_images_dir"].as<std::string>();
			this->createTargetDir(labeled_images_dir_, "");
		}
	}
	
	if (node_["deep_learning"]) {
		if (node_["deep_learning"]["class_names"]) {
			std::string annotation_filename = node_["deep_learning"]["annotation_filename"] ? node_["deep_learning"]["annotation_filename"].as<std::string>() : "";
			if (annotation_filename != "" && output_dir_ != "") {
				annotation_filename = output_dir_ + "/" + annotation_filename;
			}
			this->initClassNames(node_["deep_learning"]["class_names"], annotation_filename);
		}
	}
	
	if (node_["input"]) {
		this->findVideoFiles(node_["input"]);
	}
}

void YamlLoader::createTargetDir(std::string dir, std::string subdir)
{
	std::cout << " ** Creating target dir: " << utils::colorText(TextType::SUCCESS, dir) << std::endl;

	boost::filesystem::path target = dir;
	
	bool is_exist = boost::filesystem::exists(target);
	
	if (!is_exist) {
		boost::filesystem::create_directories(target);
		std::cout << "  -- Created an output dir: " << utils::colorText(TextType::SUCCESS, dir) << std::endl;
	}
	
	if (subdir != "") {
		patched_dir_ = dir + "/" + subdir;
		boost::filesystem::path target2 = patched_dir_;
		is_exist = boost::filesystem::exists(target2);
		if (!is_exist) {
			boost::filesystem::create_directories(target2);
			std::cout << "  -- Created an image dir: " << utils::colorText(TextType::SUCCESS, patched_dir_) << std::endl;
		}
	}
}

void YamlLoader::initClassNames(YAML::Node data, std::string filename)
{
	if (int(data.size()) > 0) {
		
		std::ofstream myfile;
		if (filename != "") {
			myfile.open(filename);
			std::cout << " ** Created an annotation file: " << utils::colorText(TextType::SUCCESS, filename) << std::endl;
		}
		
		classnames_.clear();
		std::stringstream ss;
		for (int i=0; i<data.size(); i++) {
			std::string name = data[i].as<std::string>();
			MyLabelInfo info;
			info.id = i;
			info.color = cv::Scalar(rand() % 200, rand() % 200, rand() % 200);
			classnames_.insert(std::pair<std::string, MyLabelInfo>(name, info));
			ss << name << "(" << i << ")  ";
			if (myfile.is_open()) {
				myfile << name << "\n";
			}
		}
		std::cout << " ** Init classnames: " << utils::colorText(TextType::SUCCESS, ss.str()) << std::endl;
		if (myfile.is_open()) {
			myfile.close();
		}
	}
}

void YamlLoader::findVideoFiles(YAML::Node data)
{
	std::vector<std::string> subfix_list;
	auto it1 = data["video_subfix"];
	auto it2 = data["video_dir"];
	
	if (it1) {
		if (it1.size() > 0) {
			for (int i=0; i<it1.size(); i++) {
				subfix_list.push_back(it1[i].as<std::string>());
			}
		}
	}
	
	if (it2 && subfix_list.size() > 0) {
		std::string path = it2.as<std::string>();
		std::cout << " ** Checking videos from : " << utils::colorText(TextType::SUCCESS, path) << std::endl;
		if (boost::filesystem::exists(path)) {
			std::stringstream ss;
			for (auto &p : boost::filesystem::recursive_directory_iterator(path)) {
				std::string filename = p.path().c_str();
				bool is_valid_video_filename = false;
				for (int i=0; i<subfix_list.size() && !is_valid_video_filename; i++) {
					is_valid_video_filename = (filename.find(subfix_list[i]) != std::string::npos);
				}
				if (is_valid_video_filename) {
					std::string id = std::to_string(video_files_.size());
					id = "V" + utils::getStrId(video_files_.size(), 6, '0');
					video_files_.insert(std::pair<std::string, std::string>(id, filename));
									
					ss << "    -- " << utils::colorText(TextType::INFO, id + ": " + filename) << "\n";
				}
			}
			std::cout << "    -- Found " << utils::colorText(TextType::SUCCESS, std::to_string(int(video_files_.size())) + " files") << "\n" << ss.str() << std::endl;
		} else {
			std::cout << " ** " << utils::colorText(TextType::DANGER_B, "Invalid directory for input path: " + path) << std::endl;
		}
	}
}




