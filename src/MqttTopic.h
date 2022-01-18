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


class MqttTopic
{
public:

	MqttTopic();

	bool CheckValidity();

	std::string name;
	std::string broker_name;
	std::string mqtt_payload_name;
	std::string mqtt_ecal_type_name;
	std::string static_ecal_type_name;
	std::string mqtt_ecal_type_descriptor;
	std::string ecal_out_topic_name;
	int qos;
};

void operator>> (const YAML::Node& node, MqttTopic& mqtt_topic);
