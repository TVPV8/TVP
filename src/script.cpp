// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "script.h"
#include <filesystem>
#include "configmanager.h"

extern LuaEnvironment g_luaEnvironment;
extern ConfigManager g_config;

bool ScriptEvent::loadScript(const std::string& scriptFile)
{
	if (!scriptInterface || scriptId != 0) {
		std::cout << "Failure: [ScriptEvent::loadScript] scriptInterface == nullptr. scriptFile = " << scriptFile
		          << std::endl;
		return false;
	}

	if (scriptInterface->loadFile(scriptFile) == -1) {
		std::cout << "[Warning - ScriptEvent::loadScript] Can not load script. " << scriptFile << std::endl;
		std::cout << scriptInterface->getLastLuaError() << std::endl;
		return false;
	}

	int32_t id = scriptInterface->getEvent(getScriptEventName());
	if (id == -1) {
		std::cout << "[Warning - ScriptEvent::loadScript] ScriptEvent " << getScriptEventName() << " not found. "
		          << scriptFile << std::endl;
		return false;
	}

	scriptId = id;
	return true;
}

bool ScriptEvent::loadCallback()
{
	if (!scriptInterface || scriptId != 0) {
		std::cout << "Failure: [ScriptEvent::loadCallback] scriptInterface == nullptr. scriptid = " << scriptId
		          << std::endl;
		return false;
	}

	int32_t id = scriptInterface->getEvent();
	if (id == -1) {
		std::cout << "[Warning - ScriptEvent::loadCallback] ScriptEvent " << getScriptEventName() << " not found. "
		          << std::endl;
		return false;
	}

	scripted = true;
	scriptId = id;
	return true;
}

bool CallBack::loadCallBack(LuaScriptInterface* interface, const std::string& name)
{
	if (!interface) {
		std::cout << "Failure: [CallBack::loadCallBack] scriptInterface == nullptr" << std::endl;
		return false;
	}

	scriptInterface = interface;

	int32_t id = scriptInterface->getEvent(name);
	if (id == -1) {
		std::cout << "[Warning - CallBack::loadCallBack] ScriptEvent " << name << " not found." << std::endl;
		return false;
	}

	scriptId = id;
	loaded = true;
	return true;
}

Scripts::Scripts() : scriptInterface("Scripts Interface") { scriptInterface.initState(); }

Scripts::~Scripts() { scriptInterface.reInitState(); }

bool Scripts::loadScripts(std::string folderName, bool isLib, bool reload)
{
	namespace fs = std::filesystem;

	const auto dir = fs::current_path() / "data" / folderName;
	if (!fs::exists(dir) || !fs::is_directory(dir)) {
		std::cout << "[Warning - Scripts::loadScripts] Can not load folder '" << folderName << "'." << std::endl;
		return false;
	}

	fs::recursive_directory_iterator endit;
	std::vector<fs::path> v;
	std::string disable = ("#");
	for (fs::recursive_directory_iterator it(dir); it != endit; ++it) {
		auto fn = it->path().parent_path().filename();
		if ((fn == "lib" && !isLib) || fn == "events") {
			continue;
		}
		if (fs::is_regular_file(*it) && it->path().extension() == ".lua") {
			size_t found = it->path().filename().string().find(disable);
			if (found != std::string::npos) {
				if (g_config.getBoolean(ConfigManager::SCRIPTS_CONSOLE_LOGS)) {
					std::cout << "> " << it->path().filename().string() << " [disabled]" << std::endl;
				}
				continue;
			}
			v.push_back(it->path());
		}
	}
	sort(v.begin(), v.end());
	std::string redir;
	for (auto it = v.begin(); it != v.end(); ++it) {
		const std::string scriptFile = it->string();
		if (!isLib) {
			if (redir.empty() || redir != it->parent_path().string()) {
				auto p = fs::path(it->relative_path());
				if (g_config.getBoolean(ConfigManager::SCRIPTS_CONSOLE_LOGS)) {
					std::cout << ">> [" << p.parent_path().filename() << "]" << std::endl;
				}
				redir = it->parent_path().string();
			}
		}

		if (scriptInterface.loadFile(scriptFile) == -1) {
			std::cout << "> " << it->filename().string() << " [error]" << std::endl;
			std::cout << "^ " << scriptInterface.getLastLuaError() << std::endl;
			continue;
		}

		if (g_config.getBoolean(ConfigManager::SCRIPTS_CONSOLE_LOGS)) {
			if (!reload) {
				std::cout << "> " << it->filename().string() << " [loaded]" << std::endl;
			} else {
				std::cout << "> " << it->filename().string() << " [reloaded]" << std::endl;
			}
		}
	}

	return true;
}
