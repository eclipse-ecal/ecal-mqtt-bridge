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

#include <mosquittopp.h>
#include <mosquitto.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <ecal/ecal.h>
#include <atomic>
#include <limits>

#include "utils.h"
#include "yaml-cpp/yaml.h"

#include "Broker.h"


enum ErrorTypes {
    MOSQ_STR_ERROR, MOSQ_CONN_ERROR, MOSQ_REASON_ERROR
};

/**
 * @brief A Bridge that routes messages from MQTT to eCAL and vice versa.
 *
 * On creation, the Bridge initializes mosquitto (for MQTT) and eCAL.
 * Subscribers and publishers on eCAL and MQTT side are created for a list of
 * topics. Only messages from those topics will be routed to the other side.
 */
class Bridge : public mosqpp::mosquittopp
{
public:
  /**
   * @brief constructor for the MQTT <-> eCAL Bridge
   *
   * This constructor completely initializes eCAL and MQTT. The Bridge will
   * create subsribers and publishers on both sides according to the given topic
   * maps. If the initialization was successfull, the Bridge will immediatelly
   * start to route all messages that appear on those topics.
   * You can check @code(Bridge::is_initialized) wether the initialization was
   * successfull or not.
   *
   * @param argc                command line argument counter for the eCAL API
   * @param argv                command line parameters for the eCAL API
   * @param broker              settings for mosquitto that are used to connect to the broker
   * @param mqtt2ecal_messages  a vector containg topics to send to eCAL
   * @param ecal2mqtt_messages  a vector containg topics to send to MQTT
   * @param general_settings    general settings for bridge
   * @param verbose             print all logging information from MQTT
   */
  Bridge(int argc, char** argv,const Broker& broker,const std::vector<MqttTopic>& mqtt2ecal_topics,const std::vector<EcalTopic>& ecal2mqtt_topics, const GeneralSettings& general_settings, bool verbose);

  ~Bridge(void);
  /**
   * @brief routes the eCAL message to the according MQTT topic
   *
   * @param topic_name the eCAL channel name of the received message
   * @param data the message data
   */
  void ecalMessageReceived(const char* topic_name, const struct eCAL::SReceiveCallbackData* data);

  bool isInitialized() const;
  bool isConnectedToMqttBroker() const;
  int  getMqttRxCounter() const;
  int  getEcalRxCounter() const;
  bool tryReconnectMqtt();

private:
  const GeneralSettings                     general_settings;
  const std::vector<MqttTopic>              mqtt2ecal_topics;
  const std::vector<EcalTopic>              ecal2mqtt_topics;
  const Broker                              broker_settings;

  std::map<std::string, size_t>             from_mqtt_desc_hash;
  std::map<std::string, size_t>             from_mqtt_type_hash;

  std::mutex                                mqtt_desc_mtx;
  std::mutex                                mqtt_type_mtx;

  std::map<std::string, std::string>        mqtt_descriptor_topics;
  std::map<std::string, std::string>        mqtt_type_topics;

  std::thread                               mqtt_desc_thread;
  std::atomic<bool>                         mqtt_desc_thread_active;

  std::vector<eCAL::CSubscriber*>           ecal_subscribers;
  std::map<std::string, eCAL::CPublisher*>  ecal_publishers;

  std::hash<std::string>                    hasher;

  std::atomic<bool>                         is_initialized;
  std::atomic<bool>                         is_connected_to_mqtt_broker;
  std::atomic<bool>                         loop_started;
  int                                       mqtt_rx_counter;
  int                                       ecal_rx_counter;

  const bool                                verbose;
 

  /**
   * @brief Routes the MQTT message to the according eCAL channel.
   *
   * @param message the MQTT message
   */
  void on_message(const struct mosquitto_message *message) override;

  /**
   * @brief Callback function for the mosquitto connection.
   * This function creates the MQTT Subscribers, as we cannot do that before
   * the bridge is connected to the broker.
   *
   * @param rc the error code of the connection-attempt
   */
  void on_connect(int rc) override;

  /**
   * @brief Prints the reason for the disconnect to the console
   *
   * @param rc the error code of the disconnect
   */
  void on_disconnect(int rc) override;

  /**
   * @brief Prints the Mosquitto log to the console
   *
   * @param level the log message level from the values: MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
   * @param str   the message string
   */
  void on_log(int level, const char *str) override;

  /**
   * @brief Initializes eCAL.
   *
   * This function initializes eCAL by initializing the API and creating
   * publishers and subscribers to route data from eCAL to MQTT side and vice
   * versa.
   *
   * @param argc              command line argument counter for the eCAL API
   * @param argv              command line parameters for the eCAL API
   * @param ecal_settings      Settings for eCAL
   *
   * @return True if eCAL was initialized successfully.
   */
  bool initEcal(int argc, char** argv);

  /**
   * @brief Initializes Mosquitto.
   *
   * This function initializes mosquitto and connects to the broker.
   *
   * @param mosquitto_settings  settings for mosquitto that are used to connect to the broker
   *
   * @return True if mosquitto was initialized successfully.
   */
  bool initMqtt();
  

  void onPublisherRegistration(const char* sample_, int sample_size_);

  void descriptorUpdateLoop();

  void printVerbose(const std::string & output_, const int status_code_ = std::numeric_limits<int>::max()) const;
  void printError(const std::string & output_, const int errorcode_ = std::numeric_limits<int>::max(), const int error_type = -1) const;

  bool exclusiveLoopStart();

  bool connectOrReconnect(bool ignore_error = false);


  void initialize(int argc, char** argv);

};

