// Copyright 2023 The Forgotten Server Authors and Alejandro Mujica for many specific source code changes, All rights
// reserved. Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#pragma once

#include <boost/asio.hpp>

class Signals
{
	boost::asio::signal_set set;

public:
	explicit Signals(boost::asio::io_context& ioc);

private:
	void asyncWait();
};
