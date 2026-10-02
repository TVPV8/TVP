// Copyright 2022 Raul Sanpedro. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <fstream>

struct Position;

/// <summary>
/// Class used to write binary & text script files to be used by the game server.
/// </summary>
class ScriptWriter
{
public:
	explicit ScriptWriter() = default;
	~ScriptWriter();

	/// <summary>
	/// Open a file for writing.
	/// </summary>
	/// <param name="filename"></param>
	/// <param name="append"></param>
	/// <returns>True if the file was created successfully.</returns>
	bool open(const std::string& filename, bool append = false);

	/// <summary>
	/// Closes the file stream.
	/// </summary>
	void close();

	void writePosition(const Position& pos);
	void writeNumber(int64_t number);
	void writeText(const std::string& str);
	void writeString(const std::string& str);
	void writeLine(const std::string& str);
	void writeLine();

	static std::string prepString(const std::string& str);

private:
	std::ofstream file;

	std::ostringstream buffer;
};