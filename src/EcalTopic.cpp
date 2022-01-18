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

#include "EcalTopic.h"

EcalTopic::EcalTopic()
{
	is_set_retain_flag = false;
	retain_flag = false;
	qos = -1;
}

bool EcalTopic::CheckValidity()
{
	if (broker_name.empty() || ecal_topic_name.empty() || mqtt_out_payload_name.empty())
		return false;

	// check if qos is in range [0,2]
	if (qos < 0 && qos > 2)
		return false;
	return true;
}

void operator>>(const YAML::Node& node, EcalTopic& ecal_topic)
{
	try
	{
		for (const auto& kv : node)
		{
			ecal_topic.name = kv.first.as<std::string>();
			break;
		}
		if (node["broker_name"])
		{
			if (node["broker_name"].as<std::string>().compare("null") != 0)
				ecal_topic.broker_name = node["broker_name"].as<std::string>();
		}
		if (node["ecal_topic_name"])
		{
			if (node["ecal_topic_name"].as<std::string>().compare("null") != 0)
				ecal_topic.ecal_topic_name = node["ecal_topic_name"].as<std::string>();
		}
		if (node["mqtt_out_payload_name"])
		{
			if (node["mqtt_out_payload_name"].as<std::string>().compare("null") != 0)
				ecal_topic.mqtt_out_payload_name = node["mqtt_out_payload_name"].as<std::string>();
		}
		if (node["mqtt_out_type_name"])
		{
			if (node["mqtt_out_type_name"].as<std::string>().compare("null") != 0)
				ecal_topic.mqtt_out_type_name = node["mqtt_out_type_name"].as<std::string>();
		}
		if (node["mqtt_out_descriptor"])
		{
			if (node["mqtt_out_descriptor"].as<std::string>().compare("null") != 0)
				ecal_topic.mqtt_out_descriptor = node["mqtt_out_descriptor"].as<std::string>();
		}
		if (node["retain_flag"])
		{
			ecal_topic.is_set_retain_flag = true;
			ecal_topic.retain_flag = node["retain_flag"].as<bool>();
		}
		if (node["qos"])
		{
			ecal_topic.qos = node["qos"].as<int>();
		}
	}
	catch (const YAML::BadConversion& e)
	{
		std::cout << "  ---- " << e.what() << " ----" << std::endl;
		abort();
	}
}
