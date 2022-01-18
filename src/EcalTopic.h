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


class EcalTopic
{
public:

	EcalTopic();

	bool CheckValidity();

	std::string name;
	std::string broker_name;
	std::string ecal_topic_name;
	std::string mqtt_out_payload_name;
	std::string mqtt_out_type_name;
	std::string mqtt_out_descriptor;
	bool retain_flag;
	bool is_set_retain_flag;
	int qos;
};

void operator>> (const YAML::Node& node, EcalTopic& ecal_topic);
