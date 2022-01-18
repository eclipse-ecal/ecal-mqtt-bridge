/* ========================= MQTT2eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= MQTT2eCAL LICENSE =================================
*/

#pragma once

#include "yaml-cpp/yaml.h"
#include <vector>
#include <iostream>

class Broker
{
public:

	Broker();

	bool CheckValidity();

	std::string name;
	std::string host;
	int port;
	std::string user;
	std::string password;
	std::string id;
	bool randomize_id;
	int default_qos;
	int keep_alive;
	bool default_retain_flag;
	std::string bind_ip;

	bool use_ssl;
	std::string ca_file;
	std::string cert_file;
	std::string key_file;
	bool check_host_name_match;
	std::string tls_version;
	std::vector<std::string> tls_ciphers;

	bool ssl_use_psk;
	std::string psk_id;
	std::string psk;
	std::vector<std::string> psk_ciphers;

	bool ssl_verify_server;
	bool ignore_error_first_connect;
};

void operator>> (const YAML::Node& node, Broker& broker);
