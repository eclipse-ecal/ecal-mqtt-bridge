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

#include "Bridge.h"
#include "stringutils.h"

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <memory>
#include <cstdlib>
#include <time.h>

#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX


void usage()
{
  std::cout << "Parameters:" << std::endl;
  std::cout << "-h            --help           Display this help" << std::endl;
  std::cout << "-c=PATH       --config=PATH    Use the path to a yaml file to load the config" << std::endl;
  std::cout << "-v            --verbose        Print all logging information from MQTT" << std::endl;
}

void setAlgoState(char state_, const char *infoText_) 
{
    eCAL_Process_eSeverity severity;
    switch (state_)
    {
    case 2: // ok
        severity = proc_sev_healthy;
        break;
    case 1: // warnings
        severity = proc_sev_warning;
        break;
    case 0: // error
        severity = proc_sev_critical;
        break;
    default:
        severity = proc_sev_unknown;
    }
    eCAL::Process::SetState(severity, proc_sev_level1, infoText_);
};

bool CheckYamlValidity(std::map<std::string,Broker>& brokers,
                       std::map<std::string,MqttTopic>& mqtt2ecal_topics,
                       std::map<std::string,EcalTopic>& ecal2mqtt_topics)
{
    // Check if we have any valid brokers
    if (brokers.size() == 0)
    {
        printError("No brokers configured");
        return false;
    }

    // Check if the broker name is valid for mqtt --> ecal topics
    // If not, delete from mqtt2ecal topics list
    for (auto it = mqtt2ecal_topics.cbegin(); it != mqtt2ecal_topics.cend();)
    {
        if (brokers.count(it->second.broker_name) != 1)
        {
            printError("No broker with name " + it->second.broker_name + " configured for topic " + it->first);
            // delete from map
            mqtt2ecal_topics.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    // Check if the broker name is valid for ecal --> mqtt topics
    // If not, delete from ecal2mqqt topics list
    for (auto it = ecal2mqtt_topics.cbegin(); it != ecal2mqtt_topics.cend();)
    {
        if (brokers.count(it->second.broker_name) != 1)
        {
            printError("No broker with name " + it->second.broker_name + " configured for topic " + it->first);
            // delete from map
            ecal2mqtt_topics.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    // Throw out any brokers that are not used
    bool found = false;
    for (auto it = brokers.cbegin(); it != brokers.cend();)
    {
        for (auto topic : mqtt2ecal_topics)
        {
            if (topic.second.broker_name == it->second.name)
            {
                found = true;
            }
        }
        for (auto topic : ecal2mqtt_topics)
        {
            if (topic.second.broker_name == it->second.name)
            {
                found = true;
            }

        }
        if (!found)
        {
            printError("Broker " + it->second.name + " is not used");
            brokers.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    // if no quality of service is defined to eCAL -> MQTT, use the default one from the corresponding broker
    // if no retain_flag is set, use the default one from the corresponding broker
    for (auto topic : ecal2mqtt_topics)
    {
        if (!topic.second.is_set_retain_flag || topic.second.qos == -1)
        {
            for (auto broker : brokers)
            {
                if (broker.second.name == topic.second.broker_name)
                {
                    if (!topic.second.is_set_retain_flag)
                    {
                        topic.second.retain_flag = broker.second.default_retain_flag;
                    }
                    if (topic.second.qos == -1)
                    {
                        topic.second.qos = broker.second.default_qos;
                    }
                }
            }

        }
    }

    // if no quality of service is defined to MQTT -> eCAL, use the default one from the corresponding broker
    for (auto topic : mqtt2ecal_topics)
    {
        // if the qos is not defined, iterate through all brokers, find the corresponding one and set the qos to the default one from the broker
        if (topic.second.qos == -1)
        {
            for (auto broker : brokers)
            {
                if (broker.second.name == topic.second.broker_name)
                {
                    topic.second.qos = broker.second.default_qos;
                }
            }

        }
    }
    return true;
}

void run(int argc, char** argv, const std::string& path_to_config, bool verbose)
{
    GeneralSettings general_settings;
    std::map<std::string, Broker>    brokers;
    std::map<std::string, MqttTopic> mqtt2ecal_topics;
    std::map<std::string, EcalTopic> ecal2mqtt_topics;

    YAML::Node loaded_file;
    YAML::Node gateway;
    printOutput("************************************************************************");
    printOutput("Starting to parse yaml file...");
    try 
    {
        loaded_file = YAML::LoadFile(path_to_config);
        gateway = loaded_file["gateway"];
    }
    catch (std::exception& e)
    {
        printError(e.what());
        return;
    }

    if (gateway["hide_secrets"])
    {
        general_settings.hide_secrets = gateway["hide_secrets"].as<bool>();
    }
    if (gateway["mqtt_protocol_version"])
    {
        if(gateway["mqtt_protocol_version"].as<std::string>().compare("null") != 0)
            general_settings.mqtt_protocol_version = gateway["mqtt_protocol_version"].as<std::string>();
    }
    if (gateway["ecal_process_name"])
    {
        if (gateway["ecal_process_name"].as<std::string>().compare("null") != 0)
            general_settings.ecal_process_name = gateway["ecal_process_name"].as<std::string>();
    }

    YAML::Node yaml_brokers   = gateway["brokers"];
    YAML::Node yaml_mqtt2ecal = gateway["mqtt2ecal"];
    YAML::Node yaml_ecal2mqtt = gateway["ecal2mqtt"];

    // fill out brokers map
    for (auto current_broker: yaml_brokers)
    {
        Broker broker;
        current_broker >> broker;

        // keep only valid brokers
        if (broker.CheckValidity())
        {
            brokers.insert({broker.name, broker });
        }
        else
        {
            printError("Broker " + broker.name + " is not valid");
        }
    }

    // fill out mqtt2ecal map
    for (auto current_mqtt_topic : yaml_mqtt2ecal)
    {
        MqttTopic mqtt_topic;
        current_mqtt_topic >> mqtt_topic;

        // keep only valid topics
        if (mqtt_topic.CheckValidity())
        {
            mqtt2ecal_topics.insert({mqtt_topic.name, mqtt_topic });
        }
        else
        {
            printError("Topic " + mqtt_topic.name + " is not valid");
        }
    }

    // fill out ecal2mqtt map
    for (auto current_ecal_topic : yaml_ecal2mqtt)
    {
        EcalTopic ecal_topic;
        current_ecal_topic >> ecal_topic;

        // keep only valid topics
        if (ecal_topic.CheckValidity())
        {
            ecal2mqtt_topics.insert({ecal_topic.name, ecal_topic });
        }
        else
        {
            printError("Topic " + ecal_topic.name + " is not valid");
        }
    }

    if (!CheckYamlValidity(brokers, mqtt2ecal_topics, ecal2mqtt_topics))
    {
        return;
    }
  
    if (verbose)
    {
        printOutput("************************************************************************");
        printOutput(add_spacing("eCAL -> MQTT"));
        printOutput("Channels:");
        printTopicMapEcal2Mqtt(ecal2mqtt_topics);
        printOutput("Type:");
        printTypesMapEcal2Mqtt(ecal2mqtt_topics);
        printOutput("Descriptor info:");
        printDescriptorsMapEcal2Mqtt(ecal2mqtt_topics);
        printOutput("************************************************************************");
        printOutput(add_spacing("MQTT -> eCAL"));
        printOutput("Channels:");
        printTopicMapMqtt2Ecal(mqtt2ecal_topics);
        printOutput("Type:");
        printTypesMapMqtt2Ecal(mqtt2ecal_topics);
        printOutput("Descriptor info:");
        printDescriptorsMapMqtt2Ecal(mqtt2ecal_topics);
        printOutput("************************************************************************");
        printOutput(add_spacing("General settings"));
        printGeneralSettings(general_settings);

    }

  char state = 1;
  std::string info = "connecting";
  setAlgoState(state, info.c_str());

  std::list<std::unique_ptr<Bridge>> list_of_bridges;
  for (std::pair<std::string, Broker> broker : brokers)
  {
      // filter topics that only use the current broker to send to the bridge
      std::vector<MqttTopic> temp_mqtt2ecal;
      std::vector<EcalTopic> temp_ecal2mqtt;

      for (auto topic : mqtt2ecal_topics)
      {
          if (topic.second.broker_name == broker.first)
          {
              temp_mqtt2ecal.push_back(topic.second);
          }
      }
      for (auto topic : ecal2mqtt_topics)
      {
          if (topic.second.broker_name == broker.first)
          {
              temp_ecal2mqtt.push_back(topic.second);
          }
      }

      list_of_bridges.push_back(std::make_unique<Bridge>(argc, argv, broker.second, temp_mqtt2ecal, temp_ecal2mqtt, general_settings, verbose));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  for (auto const& bridge : list_of_bridges) 
  {
      if (bridge->isInitialized() == true)
      {
          while (eCAL::Ok() == true)
          {
              if (!bridge->isConnectedToMqttBroker())
              {
                  state = 0;
                  info = "not connected";
                  if ((!bridge->getEcalRxCounter()) && (!bridge->getMqttRxCounter()))
                  {
                      info = "not connected, trying to reconnect";
                      bridge->tryReconnectMqtt();

                  }
              }
              else if (!bridge->getEcalRxCounter() && !bridge->getMqttRxCounter())
              {
                  state = 1;
                  info = "no data exchange";
              }
              else
              {
                  state = 2;
                  info = "ok, " + std::to_string(bridge->getEcalRxCounter()) + " tx-pkts, " + std::to_string(bridge->getMqttRxCounter()) + " rx-pkts";
              }
              if (verbose == true)
              {
                  std::cout << getLogTime() << ": current status: " << info << std::endl;
              }

              setAlgoState(state, info.c_str());
              std::this_thread::sleep_for(std::chrono::milliseconds(2000));
          }
      }
      else
      {
          std::cerr << getLogTime() << ": Error when initializing. The program will now exit." << std::endl;
      }
  }
}

std::string ExePath() 
{
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) 
    {
        std::string path(dirname(result));
        return path + "/";
    }
}

std::string getArg(std::string &param)
{
  auto index = param.find('=');
  return param.substr(index+1, param.length() - index + 1);
}

int main(int argc, char** argv)
{
  std::string path_to_config = ExePath() + "settings.yaml";
  bool verbose = false;

  if (argc == 1)
  {
    // No parameters
    run(argc, argv, path_to_config, verbose);
  }
  else
  {
    if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")
    {
      // Display help
      usage();
      return 0;
    }
    else
    {
      // iterate over all parameters
      for (int i=1; i<argc; i++)
      {
        std::string param(argv[i]);
        bool valid_param = false;

        if (utils::StringUtils::startsWith(param, "-c=") || utils::StringUtils::startsWith(param, "-config="))
        {
          path_to_config = getArg(param);
          valid_param = true;
        }
        else 
        if ((param == "-v") || (param == "--verbose")) 
        {
          verbose = true;
          valid_param = true;
        }

        if (!valid_param)
        {
          std::cerr << "Unrecognized Parameter: " << param << std::endl << std::endl;
          usage();
          return 0;
        }
      }
      run(0/*argc*/, argv, path_to_config, verbose); // ecal_init fails on ecal-unknown parameter, so we have to provide no parameter
    }
  }
}
