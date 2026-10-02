// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include "luascript.h"
#include "const.h"

class TalkAction;
using TalkAction_ptr = std::unique_ptr<TalkAction>;

enum TalkActionResult_t
{
	TALKACTION_CONTINUE,
	TALKACTION_BREAK,
	TALKACTION_FAILED,
};

class TalkAction : public ScriptEvent
{
public:
	explicit TalkAction(LuaScriptInterface* interface) : ScriptEvent(interface) {}

	const std::string& getWords() const { return words; }
	const std::vector<std::string>& getWordsMap() const { return wordsMap; }
	void setWords(std::string word)
	{
		words = word;
		wordsMap.push_back(word);
	}
	std::string getSeparator() const { return separator; }
	void setSeparator(std::string sep) { separator = sep; }

	// scripting
	bool executeSay(Player* player, const std::string& words, const std::string& param, SpeakClasses type) const;

	AccountType_t getRequiredAccountType() const { return requiredAccountType; }

	void setRequiredAccountType(AccountType_t reqAccType) { requiredAccountType = reqAccType; }

	bool getNeedAccess() const { return needAccess; }

	void setNeedAccess(bool b) { needAccess = b; }

private:
	std::string getScriptEventName() const override;

	std::string words;
	std::vector<std::string> wordsMap;
	std::string separator = "\"";
	bool needAccess = false;
	AccountType_t requiredAccountType = ACCOUNT_TYPE_NORMAL;
};

class TalkActions final
{
public:
	TalkActions();
	~TalkActions();

	// non-copyable
	TalkActions(const TalkActions&) = delete;
	TalkActions& operator=(const TalkActions&) = delete;

	TalkActionResult_t playerSaySpell(Player* player, SpeakClasses type, const std::string& words) const;

	bool registerLuaEvent(TalkAction* event);
	void clear();

private:
	LuaScriptInterface& getScriptInterface();
	std::string getScriptBaseName() const;

	std::map<std::string, TalkAction> talkActions;

	LuaScriptInterface scriptInterface;
};
