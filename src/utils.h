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

#include <string>
#include <chrono>

#include "MqttTopic.h"
#include "EcalTopic.h"

struct GeneralSettings 
{
  /** The name of the eCAL process */
  bool hide_secrets;
  std::string mqtt_protocol_version;
  std::string ecal_process_name;

  GeneralSettings() :
      hide_secrets(true),
      mqtt_protocol_version("v3.1.1"),
      ecal_process_name("mqtt_ecal_bridge")
  {}
};

static std::string getLogTime()
{
	auto now = std::chrono::system_clock::now();
	return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
}


// print errors even if verbose is false
static void printError(const std::string& output_)
{
	std::cerr << getLogTime() << ": " << output_ << std::endl;
}

// print errors even if verbose is false
static void printOutput(const std::string& output_)
{
	std::cout << getLogTime() << ": " << output_ << std::endl;
}

static std::string add_spacing(std::string mystring_) {
	int spaces = (72 - static_cast<int>(mystring_.size())) / 2;
	return mystring_.insert(0, spaces, ' ');
}


static void printTopicMapMqtt2Ecal(const std::map <std::string, MqttTopic>& topic_map) 
{
    if (topic_map.size() == 0) 
    {
        printOutput("No topics for MQTT to eCAL configured");
    }
    else 
    {
        for (auto t : topic_map) 
        {
            printOutput(t.second.mqtt_payload_name + " -> " + t.second.ecal_out_topic_name);
        }
    }
}

static void printTopicMapEcal2Mqtt(const std::map <std::string, EcalTopic>& topic_map) 
{
    if (topic_map.size() == 0)
    {
        printOutput("No topics for eCAL to MQTT configured");
    }
    else
    {
        for (auto t : topic_map) 
        {
            printOutput(t.second.ecal_topic_name + " -> " + t.second.mqtt_out_payload_name);
        } 
    }
}


static void printTypesMapMqtt2Ecal(const std::map <std::string, MqttTopic>& topic_map) 
{
  if (topic_map.size() == 0) 
  {
      printOutput("No topic types for MQTT to eCAL configured");
  }
  else 
  {
    for (auto t : topic_map) 
    {
      printOutput(t.second.ecal_out_topic_name + " -> " + t.second.mqtt_ecal_type_name);
    }
  }
}

static void printTypesMapEcal2Mqtt(const std::map <std::string, EcalTopic>& topic_map)
{
    if (topic_map.size() == 0)
    {
        printOutput("No topic types for eCAL to MQTT configured");
    }
    else 
    {
        for (auto t : topic_map)
        {
            printOutput(t.second.ecal_topic_name + " -> " + t.second.mqtt_out_type_name);
        }
    }
}

static void printDescriptorsMapMqtt2Ecal(const std::map <std::string, MqttTopic>& topic_map) 
{
  if (topic_map.size() == 0) 
  {
      printOutput("No descriptor info for MQTT to eCAL configured");
  }
  else 
  {
    for (auto t : topic_map) 
    {
      printOutput(t.second.ecal_out_topic_name + " -> " + t.second.mqtt_ecal_type_descriptor);
    }
  }
}

static void printDescriptorsMapEcal2Mqtt(const std::map <std::string, EcalTopic>& topic_map) 
{
    if (topic_map.size() == 0) 
    {
        printOutput("No descriptor info for eCAL to MQTT configured");
    }
    else 
    {
        for (auto t : topic_map) {
            printOutput(t.second.ecal_topic_name + " -> " + t.second.mqtt_out_descriptor);
        }
    }
}

static void printGeneralSettings(const GeneralSettings& general_settings) 
{
    printOutput("hide_secrets: " + std::to_string(general_settings.hide_secrets));
    printOutput("mqtt_protocol_version: " + general_settings.mqtt_protocol_version);
    printOutput("ecal_process_name: " + general_settings.ecal_process_name);
}
