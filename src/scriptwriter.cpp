// Copyright 2022 Raul Sanpedro. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "scriptwriter.h"
#include "position.h"
#include "tools.h"

ScriptWriter::~ScriptWriter() { close(); }

bool ScriptWriter::open(const std::string& filename, bool append)
{
	if (append) {
		file.open(filename, std::ostream::out | std::ostream::binary | std::ostream::app);
	} else {
		file.open(filename, std::ostream::out | std::ostream::binary);
	}

	if (!file.is_open()) {
		std::cout << "[ERROR - ScriptWriter::open] Could not open file for writing '" << filename << '\'' << std::endl;
		return false;
	}

	return true;
}

void ScriptWriter::close()
{
	if (!file.is_open()) {
		return;
	}

	file.write(buffer.str().c_str(), buffer.str().size());
	file.close();
}

void ScriptWriter::writePosition(const Position& pos)
{
	buffer << '[' << pos.x << ',' << pos.y << ',' << static_cast<int32_t>(pos.z) << ']';
}

void ScriptWriter::writeNumber(int64_t number) { buffer << number; }

void ScriptWriter::writeText(const std::string& str) { buffer << str; }

void ScriptWriter::writeString(const std::string& str) { buffer << "\"" << str << "\""; }

void ScriptWriter::writeLine(const std::string& str) { buffer << str << std::endl; }

void ScriptWriter::writeLine() { buffer << std::endl; }

std::string ScriptWriter::prepString(const std::string& str)
{
	std::string copy = str;
	replaceString(copy, "\n", "\\n");
	replaceString(copy, "\"", "\\\"");
	return copy;
}
