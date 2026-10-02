// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "luascript.h"

class ScriptEvent;
using ScriptEvent_ptr = std::unique_ptr<ScriptEvent>;

class ScriptEvent
{
public:
	explicit ScriptEvent(LuaScriptInterface* interface) : scriptInterface(interface) {}
	virtual ~ScriptEvent() = default;

	bool loadScript(const std::string& scriptFile);
	bool loadCallback();

	int32_t getScriptId() const { return scriptId; }

protected:
	virtual std::string getScriptEventName() const = 0;

	bool scripted = false;
	int32_t scriptId = 0;
	LuaScriptInterface* scriptInterface = nullptr;
};

class CallBack
{
public:
	CallBack() = default;

	bool loadCallBack(LuaScriptInterface* interface, const std::string& name);

protected:
	int32_t scriptId = 0;
	LuaScriptInterface* scriptInterface = nullptr;

private:
	bool loaded = false;
};

class Scripts
{
public:
	Scripts();
	~Scripts();

	bool loadScripts(std::string folderName, bool isLib, bool reload);
	LuaScriptInterface& getScriptInterface() { return scriptInterface; }

private:
	LuaScriptInterface scriptInterface;
};
