#include "otpch.h"

#include "scriptreader.h"
#include "tools.h"

#include <boost/lexical_cast.hpp>

ScriptReader::~ScriptReader()
{
	if (recursionDepth == -1) {
		return;
	}

	for (FILE* file : files) {
		if (file) {
			fclose(file);
		}
	}
}

bool ScriptReader::loadScript(const std::string_view& filename, bool important)
{
	if (recursionDepth++ >= 3) {
		std::cout << "[Warning - ScriptReader:loadScript] Recursion depth too high " << filename << std::endl;
		return false;
	}

	files[recursionDepth] = fopen(filename.data(), "rb");
	lines[recursionDepth] = 1;
	filenames[recursionDepth] = filename;

	if (!files[recursionDepth]) {
		if (important) {
			std::cout << "[Error - ScriptReader::loadScript] Script file does not exist: " << filename << std::endl;
		}
		return false;
	}

	isGood = true;
	return true;
}

bool ScriptReader::canRead() const
{
	if (files[recursionDepth] == nullptr) return false;
	return isGood;
}

TokenType_t ScriptReader::nextToken(bool allowNegativeDigits)
{
	while (canRead()) {
		int8_t next = getc(files[recursionDepth]);
		if (next == -1) {
			token = TOKEN_ENDOFFILE;
			closeCurrentFile();
			if (recursionDepth == -1) {
				isGood = false;
				return token;
			}
			continue;
		}

		if (next == ' ' || next == '\t') {
			continue;
		} else if (next == '#') {
			while (canRead()) {
				next = getc(files[recursionDepth]);
				if (next == '\n' || next == '\r') {
					if (next == '\n') {
						lines[recursionDepth]++;
					}
					break;
				}
			}
		} else if (next == '\n' || next == '\r') {
			if (next == '\n') {
				lines[recursionDepth]++;
			}
		} else if (isalpha(next)) {
			std::ostringstream ss;
			ss << static_cast<char>(next);

			while (canRead()) {
				next = getc(files[recursionDepth]);
				if (!isalpha(next) && !isdigit(next) && next != '_') {
					ungetc(next, files[recursionDepth]);
					break;
				}

				ss << static_cast<char>(next);
			}

			token = TOKEN_IDENTIFIER;
			identifier = ss.str();
			identifier = asLowerCaseString(identifier);
			return token;
		} else if (isdigit(next)) {
			std::ostringstream ss;
			ss << static_cast<char>(next);

			while (canRead()) {
				next = getc(files[recursionDepth]);
				if (!isdigit(next)) {
					ungetc(next, files[recursionDepth]);
					break;
				}

				ss << static_cast<char>(next);
			}

			token = TOKEN_NUMBER;
			try {
				number = boost::lexical_cast<int64_t>(ss.str());
			} catch (const boost::bad_lexical_cast& e) {
				number = -1;
				error("bad number structure");
				return TOKEN_ENDOFFILE;
			}

			return token;
		} else if (next == '"') {
			std::ostringstream ss;

			while (canRead()) {
				next = getc(files[recursionDepth]);

				if (next == '\\') {
					next = getc(files[recursionDepth]);
					if (next == 'n') {
						ss << '\n';
					} else if (next == '"') {
						ss << '"';
					} else {
						ss << '\\';
					}
					continue;
				}

				if (next == '"') {
					break;
				}

				ss << next;
			}

			token = TOKEN_STRING;
			string = ss.str();
			return token;
		} else { // special-characters
			token = TOKEN_SPECIAL;
			special = next;
			if (special == '@') {
				// recursive file load
				std::ostringstream ss;
				ss << "data/npc/behavior/" << readString();
				if (!loadScript(ss.str())) {
					return token;
				}

				continue;
			}

			if (next == '>') { // greater or equals
				next = getc(files[recursionDepth]);
				if (next == '=') {
					special = 'G';
					return token;
				}

				special = '>';
				ungetc(next, files[recursionDepth]);
			} else if (next == '<') { // not equals
				next = getc(files[recursionDepth]);
				if (next == '>') {
					special = 'N';
					return token;
				} else if (next == '=') {
					special = 'L';
					return token;
				}

				special = '<';
				ungetc(next, files[recursionDepth]);
			} else if (next == '-') {
				next = getc(files[recursionDepth]);
				if (next == '>') {
					special = 'I';
					return token;
				} else if (allowNegativeDigits && isdigit(next)) {
					std::ostringstream ss;
					ss << '-' << static_cast<char>(next);

					while (canRead()) {
						next = getc(files[recursionDepth]);
						if (!isdigit(next)) {
							ungetc(next, files[recursionDepth]);
							break;
						}

						ss << static_cast<char>(next);
					}

					token = TOKEN_NUMBER;
					try {
						number = boost::lexical_cast<int64_t>(ss.str());
					} catch (const boost::bad_lexical_cast& e) {
						number = -1;
						error("bad number structure");
						return TOKEN_ENDOFFILE;
					}

					return token;
				}

				special = '-';
				ungetc(next, files[recursionDepth]);
			}

			return token;
		}
	}

	return token;
}

void ScriptReader::error(const std::string& errMessage)
{
	if (!isGood) {
		return;
	}

	std::ostringstream ss;
	ss << "[Error - ScriptReader::error]: In script file '" << filenames[recursionDepth]
	   << "':" << lines[recursionDepth] << ": " << errMessage << std::endl;
	ss << "[Error - ScriptReader::error] Token: " << static_cast<int32_t>(token)
	   << " Special: " << static_cast<int32_t>(special) << std::endl;
	std::cout << ss.str() << std::endl;
	isGood = false;
}

const std::string& ScriptReader::getIdentifier()
{
	if (token != TOKEN_IDENTIFIER) {
		error("identifier expected");
		return identifier;
	}

	return identifier;
}

const std::string& ScriptReader::getString()
{
	if (token != TOKEN_STRING) {
		error("string expected");
		return string;
	}

	return string;
}

int64_t ScriptReader::getNumber()
{
	if (token != TOKEN_NUMBER) {
		error("number expected");
		return number;
	}

	return number;
}

int8_t ScriptReader::getSpecial()
{
	if (token != TOKEN_SPECIAL) {
		error("special-char expected");
		return special;
	}

	return special;
}

Position ScriptReader::getPosition()
{
	Position pos;
	pos.x = readNumber();
	readSymbol(',');
	pos.y = readNumber();
	readSymbol(',');
	pos.z = readNumber();
	readSymbol(']');
	return pos;
}

const std::string& ScriptReader::readIdentifier()
{
	nextToken();
	return getIdentifier();
}

const std::string& ScriptReader::readString()
{
	nextToken();
	return getString();
}

int64_t ScriptReader::readNumber()
{
	nextToken(true);
	return getNumber();
}

int8_t ScriptReader::readSpecial()
{
	nextToken();
	return getSpecial();
}

int8_t ScriptReader::readSymbol(int8_t symbol)
{
	nextToken();
	if (token != TOKEN_SPECIAL || special != symbol) {
		std::ostringstream ss;
		ss << '"' << static_cast<char>(symbol) << "' expected got " << static_cast<int32_t>(token) << " token instead";
		error(ss.str());
		return special;
	}

	return special;
}

Position ScriptReader::readPosition()
{
	readSymbol('[');
	Position pos;
	pos.x = readNumber();
	readSymbol(',');
	pos.y = readNumber();
	readSymbol(',');
	pos.z = readNumber();
	readSymbol(']');
	return pos;
}

std::string ScriptReader::prepString(const std::string& str)
{
	std::string copy = str;
	replaceString(copy, "\\n", "\n");
	replaceString(copy, "\\\"", "\"");
	return copy;
}

void ScriptReader::closeCurrentFile()
{
	if (recursionDepth == -1) {
		return;
	} else {
		if (fclose(files[recursionDepth])) {
			std::cout << "[Error - ScriptReader::closeCurrentFile] Failed to close current script file." << std::endl;
		}

		files[recursionDepth] = nullptr;
		recursionDepth--;
	}
}
