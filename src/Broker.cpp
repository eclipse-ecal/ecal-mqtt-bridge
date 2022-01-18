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

#include "Broker.h"

Broker::Broker()
{
	port = 1883;
	user = "ecal2mqtt";
	id = "1234abcd";
	randomize_id = false;
	default_qos = 0;
	keep_alive = 60;
	default_retain_flag = false;

	use_ssl = false;

	check_host_name_match = true;
	tls_version = "tlsv1.2";

	ssl_use_psk = false;

	ssl_verify_server = false;
	ignore_error_first_connect = false;
}

bool Broker::CheckValidity()
{
    // host is mandatory
	if(host.empty())
		return false;

	// if ssl_use_psk is true, psk_id and psk are mandatory
	if (ssl_use_psk)
	{
		if (psk_id.empty() || psk.empty())
			return false;
	}

	// check if default_qos is in range [0,2]
	if (default_qos < 0 && default_qos > 2)
		return false;

	// check if tls_version is valid
	std::vector<std::string> possible_tls_version{ "tlsv1.2", "tlsv1.1", "tlsv1" };
	if (std::find(std::begin(possible_tls_version), std::end(possible_tls_version), tls_version) == std::end(possible_tls_version))
		return false;

	return true;
}

void operator>>(const YAML::Node& node, Broker& broker)
{ 
	try
	{
		for (const auto& kv : node)
		{
			broker.name = kv.first.as<std::string>();
			break;
		}
		if (node["host"])
		{
			if(node["host"].as<std::string>().compare("null") != 0)
				broker.host = node["host"].as<std::string>();
		}
		if (node["port"])
		{
			broker.port = node["port"].as<int>();
		}
		if (node["user"])
		{
			if (node["user"].as<std::string>().compare("null") != 0)
				broker.user = node["user"].as<std::string>();
		}
		if (node["password"])
		{
			if (node["password"].as<std::string>().compare("null") != 0)
				broker.password = node["password"].as<std::string>();
		}
		if (node["randomize_id"])
		{
			broker.randomize_id = node["randomize_id"].as<bool>();
		}
		if (broker.randomize_id)
		{
			// if randomize_id is true, randomize in any case
			srand(static_cast<unsigned int>(time(NULL)));
			broker.id = std::to_string(rand()) + std::to_string(rand() / 2);
		}
		else
		{
			// if randomize is false, take given id
			if (node["id"])
			{
				if (node["id"].as<std::string>().compare("null") != 0)
					broker.id = node["id"].as<std::string>();
			}
			// if no id is given, randomize
			if (broker.id.empty())
			{
				srand(static_cast<unsigned int>(time(NULL)));
				broker.id = std::to_string(rand()) + std::to_string(rand() / 2);
			}
		}
		if (node["default_qos"])
		{
			broker.default_qos = node["default_qos"].as<int>();
		}
		if (node["keep_alive"])
		{
			broker.keep_alive = node["keep_alive"].as<int>();
		}
		if (node["default_retain_flag"])
		{
			broker.default_retain_flag = node["default_retain_flag"].as<bool>();
		}
		if (node["bind_ip"])
		{
			if (node["bind_ip"].as<std::string>().compare("null") != 0)
				broker.bind_ip = node["bind_ip"].as<std::string>();
		}
		if (node["ignore_error_first_connect"])
		{
			broker.ignore_error_first_connect = node["ignore_error_first_connect"].as<bool>();
		}
		if (node["use_ssl"])
		{
			broker.use_ssl = node["use_ssl"].as<bool>();
		}
		if (node["ca_file"])
		{
			if (node["ca_file"].as<std::string>().compare("null") != 0)
				broker.ca_file = node["ca_file"].as<std::string>();
		}
		if (node["cert_file"])
		{
			if (node["cert_file"].as<std::string>().compare("null") != 0)
				broker.cert_file = node["cert_file"].as<std::string>();
		}
		if (node["key_file"])
		{
			if (node["key_file"].as<std::string>().compare("null") != 0)
				broker.key_file = node["key_file"].as<std::string>();
		}
		if (node["check_hostname_match"])
		{
			broker.check_host_name_match = node["check_hostname_match"].as<bool>();
		}
		if (node["ssl_verify_server"])
		{
			broker.ssl_verify_server = node["ssl_verify_server"].as<bool>();
		}
		if (node["tls_version"])
		{
			if (node["tls_version"].as<std::string>().compare("null") != 0)
				broker.tls_version = node["tls_version"].as<std::string>();
		}
		if (node["tls_ciphers"])
		{
			broker.tls_ciphers = node["tls_ciphers"].as<std::vector<std::string>>();
		}
		if (node["ssl_use_psk"])
		{
			broker.ssl_use_psk = node["ssl_use_psk"].as<bool>();
		}
		if (node["psk_id"])
		{
			if (node["psk_id"].as<std::string>().compare("null") != 0)
				broker.psk_id = node["psk_id"].as<std::string>();
		}
		if (node["psk"])
		{
			if (node["psk"].as<std::string>().compare("null") != 0)
				broker.psk = node["psk"].as<std::string>();
		}
		if (node["psk_ciphers"])
		{
			broker.psk_ciphers = node["psk_ciphers"].as<std::vector<std::string>>();
		}
	}
	catch (const YAML::BadConversion& e)
	{
		std::cout << "  ---- " << e.what() << " ----" << std::endl;
		abort();
	}
}
