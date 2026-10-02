// Copyright 2023 Alejandro Mujica, All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

enum TokenType_t : uint8_t
{
	TOKEN_ENDOFFILE,
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_SPECIAL,
};

struct Position;

class ScriptReader
{
public:
	ScriptReader() = default;
	~ScriptReader();

	bool loadScript(const std::string_view& filename, bool important = true);
	bool canRead() const;

	TokenType_t nextToken(bool allowNegativeDigits = false);
	TokenType_t getToken() const { return token; }

	void error(const std::string& errMessage);

	const std::string& getIdentifier();
	const std::string& getString();

	template <typename T>
	T getNumber()
	{
		if (token != TOKEN_NUMBER) {
			error("number expected");
			return number;
		}

		return static_cast<T>(number);
	}

	int64_t getNumber();
	int8_t getSpecial();
	Position getPosition();

	const std::string& readIdentifier();
	const std::string& readString();

	template <typename T>
	T readNumber()
	{
		nextToken(std::is_unsigned<T>());
		return static_cast<T>(getNumber());
	}

	int64_t readNumber();
	int8_t readSpecial();
	int8_t readSymbol(int8_t symbol);
	Position readPosition();

	static std::string prepString(const std::string& str);

private:
	void closeCurrentFile();

	TokenType_t token = TOKEN_ENDOFFILE;

	std::array<FILE*, 3> files{nullptr, nullptr, nullptr};
	std::array<std::string, 3> filenames{};
	std::array<int32_t, 3> lines{1, 1, 1};

	bool isGood = true;

	int32_t special = -1;
	int32_t recursionDepth = -1;
	int64_t number = -1;

	std::string identifier;
	std::string string;
};
