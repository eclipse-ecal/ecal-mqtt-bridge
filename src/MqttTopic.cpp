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

#include "MqttTopic.h"

MqttTopic::MqttTopic()
{
	qos = -1;
}

bool MqttTopic::CheckValidity()
{
	if (broker_name.empty() || mqtt_payload_name.empty() || ecal_out_topic_name.empty())
		return false;

	// check if qos is in range [0,2]
	if (qos < 0 && qos > 2)
		return false;
	return true;
}

void operator>>(const YAML::Node& node, MqttTopic& mqtt_topic)
{
	try
	{
		for (const auto& kv : node)
		{
			mqtt_topic.name = kv.first.as<std::string>();
			break;
		}
		if (node["broker_name"])
		{
			if (node["broker_name"].as<std::string>().compare("null") != 0)
				mqtt_topic.broker_name = node["broker_name"].as<std::string>();
		}
		if (node["mqtt_payload_name"])
		{
			if (node["mqtt_payload_name"].as<std::string>().compare("null") != 0)
				mqtt_topic.mqtt_payload_name = node["mqtt_payload_name"].as<std::string>();
		}
		if (node["mqtt_ecal_type_name"])
		{
			if (node["mqtt_ecal_type_name"].as<std::string>().compare("null") != 0)
				mqtt_topic.mqtt_ecal_type_name = node["mqtt_ecal_type_name"].as<std::string>();
		}
		if (node["static_ecal_type_name"])
		{
			if (node["static_ecal_type_name"].as<std::string>().compare("null") != 0)
				mqtt_topic.static_ecal_type_name = node["static_ecal_type_name"].as<std::string>();
		}
		if (node["mqtt_ecal_type_descriptor"])
		{
			if (node["mqtt_ecal_type_descriptor"].as<std::string>().compare("null") != 0)
				mqtt_topic.mqtt_ecal_type_descriptor = node["mqtt_ecal_type_descriptor"].as<std::string>();
		}
		if (node["ecal_out_topic_name"])
		{
			if (node["ecal_out_topic_name"].as<std::string>().compare("null") != 0)
				mqtt_topic.ecal_out_topic_name = node["ecal_out_topic_name"].as<std::string>();
		}
		if (node["qos"])
		{
			mqtt_topic.qos = node["qos"].as<int>();
		}
	}
	catch (const YAML::BadConversion& e)
	{
		std::cout << "  ---- " << e.what() << " ----" << std::endl;
		abort();
	}
}
