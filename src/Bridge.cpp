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

#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#include "Bridge.h"
#include "ecal/pb/ecal.pb.h"
#include <fstream>
#include<iostream>

Bridge::Bridge(int argc, char** argv,const Broker& broker, const std::vector<MqttTopic>& mqtt2ecal_topics, const std::vector<EcalTopic>& ecal2mqtt_topics, const GeneralSettings& general_settings, bool verbose)
	: mosquittopp(broker.id.c_str(), true /* clean session */)
	, is_initialized(false)
	, is_connected_to_mqtt_broker(false)
	, broker_settings(broker)
	, mqtt2ecal_topics(mqtt2ecal_topics)
	, ecal2mqtt_topics(ecal2mqtt_topics)
	, general_settings(general_settings)
	, verbose(verbose)
	, mqtt_rx_counter(0)
	, ecal_rx_counter(0)
	, mqtt_desc_thread_active(true)
	, mqtt_desc_thread(&Bridge::descriptorUpdateLoop, this)
	, loop_started(false)
{
	initialize(argc, argv);
}

void Bridge::initialize(int argc, char** argv)
{
	if (initEcal(argc, argv) == true && initMqtt() == true)
	{
		is_initialized = true;
	}
}

void Bridge::onPublisherRegistration(const char* sample_, int sample_size_)
{
	eCAL::pb::Sample sample;
	if (sample.ParseFromArray(sample_, sample_size_))
	{
	  // We have to check with a mutex if the Name of the published topic is on the list of interest
	  auto topic_name = sample.topic().tname();
	  EcalTopic current_topic;
	  bool found = false;
	  for (auto topic : ecal2mqtt_topics)
	  {
		  if (topic_name == topic.ecal_topic_name)
		  {
			  if (!topic.mqtt_out_descriptor.empty())
			  {
				  found = true;
				  current_topic = topic;
			  }
		  }
	  }
	  if (found)
	  {
	    // This is what we have to secure with a mutex
	    std::lock_guard<std::mutex> lock(mqtt_desc_mtx);
	    mqtt_descriptor_topics[current_topic.mqtt_out_descriptor] = sample.topic().tdesc();
	  }
	}
}

bool Bridge::initEcal(int argc, char** argv)
{
	printVerbose("************************************************************************");
	printVerbose(add_spacing("eCAL settings"));
	printVerbose("Process name: " + general_settings.ecal_process_name);

	if (eCAL::Initialize(argc, argv, general_settings.ecal_process_name.c_str()) == -1)
	{
		printError("Failed to initialize eCAL");
		return false;
	}
	// Create eCAL Subscribers
	auto callback = std::bind(&Bridge::ecalMessageReceived, this, std::placeholders::_1, std::placeholders::_2);
	for (auto topic : ecal2mqtt_topics)
	{
		eCAL::CSubscriber* sub = new eCAL::CSubscriber(topic.ecal_topic_name);
		sub->AddReceiveCallback(callback);
		ecal_subscribers.push_back(sub);
		printVerbose("Creating eCAL subscriber : " + topic.ecal_topic_name);
	}
	// Create eCAL Publishers
	for (auto topic : mqtt2ecal_topics)
	{
		std::string topic_type = "";
		std::string topic_desc = "";

		if (topic.static_ecal_type_name.empty())
		{
			topic_type = topic.mqtt_ecal_type_name;
		}
		else
		{
			topic_type = topic.static_ecal_type_name;
		}
		printVerbose("Creating eCAL publisher : " + topic.ecal_out_topic_name + " (" + topic_type + ")");
		ecal_publishers[topic.ecal_out_topic_name] = (new eCAL::CPublisher(topic.ecal_out_topic_name, topic_type, topic_desc));
	}

	for (auto topic : ecal2mqtt_topics)
	{
		if (!topic.mqtt_out_descriptor.empty())
		{
			// If we need to send a descriptor info via MQTT we need a monitoring info to get the descriptor string, so we work with a event + registration callback
			eCAL::Process::AddRegistrationCallback(reg_event_publisher, std::bind(&Bridge::onPublisherRegistration, this, std::placeholders::_1, std::placeholders::_2));
			break;
		}
	}
	return true;
}

void Bridge::descriptorUpdateLoop()
{
	bool found = false;
	for (auto topic : ecal2mqtt_topics)
	{
		if (!topic.mqtt_out_descriptor.empty())
		{
			found = true;
		}
	}
	if (found)
	{
		while (mqtt_desc_thread_active == true)
		{
			// Iterate through 
			if (is_initialized && is_connected_to_mqtt_broker)
			{
				std::lock_guard<std::mutex> lock(mqtt_desc_mtx);
				for (auto const& mqtt_topic : mqtt_descriptor_topics)
				{
					// iterate through topics, find the corresponding one and publish it to mqtt
					for (auto topic : ecal2mqtt_topics)
					{
						if (topic.mqtt_out_descriptor == mqtt_topic.first) 
						{
							publish(NULL, mqtt_topic.first.c_str(), static_cast<int>(mqtt_topic.second.size()), mqtt_topic.second.data(), topic.qos, topic.retain_flag);
							std::this_thread::sleep_for(std::chrono::milliseconds(10));
							break;
						}
					}
				}
			}
			for (auto counter = 0; (mqtt_desc_thread_active == true) && (counter < 50); counter++)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}
}

/* reconnect is the same as connect except for resetting the values (which are const here so it's ok)*/
bool Bridge::connectOrReconnect(bool ignore_error)
{
	int connect_err = MOSQ_ERR_CONN_PENDING;
	if (broker_settings.bind_ip.size() > 0)
	{
		connect_err = connect(broker_settings.host.c_str(), broker_settings.port, broker_settings.keep_alive, broker_settings.bind_ip.c_str());
	}
	else
	{
		connect_err = connect_async(broker_settings.host.c_str(), broker_settings.port, broker_settings.keep_alive);
	}
	if (connect_err != MOSQ_ERR_SUCCESS)
	{
		if (ignore_error == false)
		{
			printError("Failed to connect_async", connect_err, MOSQ_STR_ERROR);
			return false;
		}
		else
		{
			printVerbose("Failed to connect_async, but ignoring this (and repeating...)", connect_err);
			return true;
		}

	}
	printVerbose("Successfully connect_async", connect_err);
	return true;
}

bool Bridge::exclusiveLoopStart()
{
	if (loop_started == false)
	{
		auto connect_err = loop_start();
		if (connect_err != MOSQ_ERR_SUCCESS)
		{
			printError("Failed to start MQTT loop", connect_err, MOSQ_STR_ERROR);
			return false;
		}
		printVerbose("Successfully started MQTT loop", connect_err);
		loop_started = true;
	}
	return true;
}

void Bridge::printVerbose(const std::string& output_, const int status_code) const
{
	if (verbose == true)
	{
		if (status_code == INT_MAX)
		{
			std::cout << getLogTime() << ": " << output_ << std::endl;
		}
		else
		{
			std::cout << getLogTime() << ": " << output_ << ": " << std::to_string(status_code) << " " << strerror(status_code) << std::endl;
		}
	}
}

void Bridge::printError(const std::string& output_, const int errorcode_, const int error_type) const
{
	if (errorcode_ == INT_MAX)
	{
		std::cerr << getLogTime() << ": " << output_ << std::endl;
	}
	else
	{
		switch (error_type)
		{
		case MOSQ_STR_ERROR:
			std::cerr << getLogTime() << ": " << output_ << ": " << std::to_string(errorcode_) << " " << mosquitto_strerror(errorcode_) << std::endl;
			break;
		case MOSQ_CONN_ERROR:
			std::cerr << getLogTime() << ": " << output_ << ": " << std::to_string(errorcode_) << " " << mosquitto_connack_string(errorcode_) << std::endl;
			break;
		case MOSQ_REASON_ERROR:
			std::cerr << getLogTime() << ": " << output_ << ": " << std::to_string(errorcode_) << " " << mosquitto_reason_string(errorcode_) << std::endl;
			break;
		default:
			std::cerr << getLogTime() << ": " << output_ << ": " << std::to_string(errorcode_) << " " << strerror(errorcode_) << std::endl;
			break;
		}
	}
}

bool Bridge::initMqtt()
{
	printVerbose("************************************************************************");
	printVerbose("                         MQTT settings ");
	printVerbose("Protocol version               " + general_settings.mqtt_protocol_version);
	printVerbose("Host                           " + broker_settings.host);
	printVerbose("Port                           " + std::to_string(broker_settings.port));
	printVerbose("Keep alive                     " + std::to_string(broker_settings.keep_alive));
	printVerbose("Default qos                    " + std::to_string(broker_settings.default_qos));
	printVerbose("Default retain flag            " + std::to_string(broker_settings.default_retain_flag));
	printVerbose("ID                             " + broker_settings.id);
	printVerbose("Randomize ID                   " + std::to_string(broker_settings.randomize_id));
	printVerbose("Hide secrets                   " + std::to_string(general_settings.hide_secrets));
	printVerbose("Username                       " + broker_settings.user);
	if (general_settings.hide_secrets == true)
	{
		printVerbose("Password                       " + std::string("***"));
	}
	else
	{
		printVerbose("Password                       " + broker_settings.password);
	}
	if (broker_settings.use_ssl)
	{
		printVerbose("Use ssl                        " + std::to_string(broker_settings.use_ssl));
		printVerbose("ca file                        " + broker_settings.ca_file);
		printVerbose("cert file                      " + broker_settings.cert_file);
		printVerbose("key file                       " + broker_settings.key_file);
		printVerbose("tls version                    " + broker_settings.tls_version);
		printVerbose("tls insecure                   " + std::to_string(broker_settings.check_host_name_match));
		printVerbose("verify ssl server              " + std::to_string(broker_settings.ssl_verify_server));
		if (general_settings.hide_secrets == true)
		{
			printVerbose("tls ciphers                    " + std::string("***"));
		}
		else
		{
			std::string ciphers = "[";
			for (std::string cipher : broker_settings.tls_ciphers)
			{
				ciphers.append("'" + cipher + "', ");
			}
			ciphers = ciphers.substr(0, ciphers.size() - 2);
			ciphers.append("]");
			printVerbose("tls ciphers                    " + ciphers);
		}
	}
	if (broker_settings.ssl_use_psk)
	{
		printVerbose("tls use psk                    " + std::to_string(broker_settings.ssl_use_psk));
		if (general_settings.hide_secrets == true)
		{
			printVerbose("psk identity                   " + std::string("***"));
			printVerbose("psk                            " + std::string("***"));
			printVerbose("psk ciphers                    " + std::string("***"));
		}
		else
		{
			printVerbose("psk identity                   " + broker_settings.psk_id);
			printVerbose("psk                            " + broker_settings.psk);
			std::string ciphers = "[";
			for (std::string cipher : broker_settings.psk_ciphers)
			{
				ciphers.append("'" + cipher + "', ");
			}
			ciphers = ciphers.substr(0, ciphers.size() - 2);
			ciphers.append("]");
			printVerbose("psk ciphers                    " + ciphers);
		}
	}
	printVerbose("Binding IP                     " + broker_settings.bind_ip);
	printVerbose("Ignore error on first connect  " + std::to_string(broker_settings.ignore_error_first_connect));

	printVerbose("************************************************************************");


	//************************ Initialize mosquitto lib *************************************/
	printVerbose("Initializing mosqpp lib");
	auto connect_err = mosqpp::lib_init();
	if (connect_err != MOSQ_ERR_SUCCESS)
	{
		printError("Failed initialize mosqpp lib", connect_err, MOSQ_STR_ERROR);
		return false;
	}
	printVerbose("Successfully initialized mosqpp lib", connect_err);

	//************************ Mosquitto library version *************************************/
	int major = 0;
	int minor = 0;
	int revision = 0;
	connect_err = mosqpp::lib_version(&major, &minor, &revision);
	if (connect_err == 0)
	{
		printError("Failed to get lib version");
		return false;
	}
	printVerbose("Using mosquitto lib " + std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(revision));

	//************************ mqtt protocol *************************************/
	int mqtt_version = 0;
	if (general_settings.mqtt_protocol_version == "v3.1.1")
	{
		mqtt_version = MQTT_PROTOCOL_V311;
	}
	else if (general_settings.mqtt_protocol_version == "v3.1")
	{
		mqtt_version = MQTT_PROTOCOL_V31;
	}
	if (mqtt_version == 0)
	{
		printError("Failed to set mqtt protocol version, unknown parameter");
		return false;
	}
	else
	{
		connect_err = opts_set(MOSQ_OPT_PROTOCOL_VERSION, static_cast<void*>(&mqtt_version));
		if (connect_err != MOSQ_ERR_SUCCESS)
		{
			printError("Failed to set mqtt protocol version", connect_err, MOSQ_STR_ERROR);
			return false;
		}
		printVerbose("Successfully setting mqtt protocol version");
	}

	//************************ username and password *************************************/
	if ((broker_settings.user.size() > 0))
	{
		printVerbose("Setting username and password");
		if (broker_settings.password.size() > 0)
		{
			connect_err = username_pw_set(broker_settings.user.c_str(), broker_settings.password.c_str());
		}
		else
		{
			// Send only the username
			connect_err = username_pw_set(broker_settings.user.c_str(), NULL);
		}

		if (connect_err != MOSQ_ERR_SUCCESS)
		{
			printError("Failed to set username and password", connect_err, MOSQ_STR_ERROR);
			return false;
		}
		printVerbose("Successfully set username and password", connect_err);
	}

	//************************ TLS *************************************/

	// use SSL only if use_ssl is true
	if (broker_settings.use_ssl)
	{
		if (broker_settings.ca_file.size() > 0)
		{
			printVerbose("Setting TLS...");
			std::ifstream ca_file_stream(broker_settings.ca_file);
				if (!ca_file_stream.good()) 
				{
					printVerbose("File " + broker_settings.ca_file + " doesn't exist or have reading rights!");
					return false;
				}

			if (broker_settings.cert_file.size() > 0)
			{
				std::ifstream cert_file_stream(broker_settings.cert_file);
					if (!cert_file_stream.good())
					{
						printVerbose("File " + broker_settings.cert_file + " doesn't exist or have reading rights!");
						return false;
					}
			}
			if (broker_settings.key_file.size() > 0)
			{
				std::ifstream key_file_stream(broker_settings.key_file);
					if (!key_file_stream.good())
					{
						printVerbose("File " + broker_settings.key_file + " doesn't exist or have reading rights!");
						return false;
					}
			}

			connect_err = tls_set(broker_settings.ca_file.c_str(), NULL /*ca path*/, broker_settings.cert_file.size() > 0 ? broker_settings.cert_file.c_str() : NULL, broker_settings.key_file.size() > 0 ? broker_settings.key_file.c_str() : NULL, NULL /*password insert function*/);
			if (connect_err != MOSQ_ERR_SUCCESS)
			{
				printError("Failed to set TLS certificates", connect_err, MOSQ_STR_ERROR);
				return false;
			}
			connect_err = tls_insecure_set(broker_settings.check_host_name_match);   // disable / enable host name verification
			if (connect_err != MOSQ_ERR_SUCCESS)
			{
				printError("Failed to set TLS host name checking", connect_err, MOSQ_STR_ERROR);
				return false;
			}
			int cert_check = broker_settings.ssl_verify_server ? 1 /*SSL_VERIFY_PEER*/ : 0 /*SSL_VERIFY_NONE*/;

			// Convert from vector of string to char* using as separator a colon
			const char* tls_ciphers;
			std::string temp_ciphers;
			if (broker_settings.tls_ciphers.size() > 0)
			{
				std::ostringstream os;
				std::copy(broker_settings.tls_ciphers.begin(), broker_settings.tls_ciphers.end() - 1,
					std::ostream_iterator<std::string>(os, ":"));
				os << *broker_settings.tls_ciphers.rbegin();
				temp_ciphers = os.str();
				tls_ciphers = temp_ciphers.c_str();
			}
			else
			{
				tls_ciphers = NULL;
			}
	
			connect_err = tls_opts_set(cert_check, broker_settings.tls_version.c_str(), tls_ciphers);       //   tlsv1.2, tlsv1.1 and tlsv1
			if (connect_err != MOSQ_ERR_SUCCESS)
			{
				printError("Failed to set TLS options", connect_err, MOSQ_STR_ERROR);
				return false;
			}
			printVerbose("Sucessfully set TLS", connect_err);
		}
	}
	else if (broker_settings.ssl_use_psk == true)
	{
		const char* psk = broker_settings.psk.size() > 0 ? broker_settings.psk.c_str() : NULL;
		const char* identity = broker_settings.psk_id.size() > 0 ? broker_settings.psk_id.c_str() : NULL;
		// Convert from vector of string to char* using as separator a colon
		const char* psk_ciphers;
		std::string temp_ciphers;
		if (broker_settings.psk_ciphers.size() > 0)
		{
			std::ostringstream os;
			std::copy(broker_settings.psk_ciphers.begin(), broker_settings.psk_ciphers.end() - 1,
				std::ostream_iterator<std::string>(os, ":"));
			os << *broker_settings.psk_ciphers.rbegin();
			temp_ciphers = os.str();
			psk_ciphers = temp_ciphers.c_str();
		}
		else
		{
			psk_ciphers = NULL;
		}
		connect_err = tls_psk_set(psk, identity, psk_ciphers);
		if (connect_err != MOSQ_ERR_SUCCESS)
		{
			printError("Failed to set TLS PSK", connect_err, MOSQ_STR_ERROR);
			return false;
		}
		printVerbose("Sucessfully set TLS PSK", connect_err);
	}

	//************************ host ip and port *************************************/
	if ((broker_settings.host.size() == 0) || (broker_settings.port == 0))
	{
		printError("Failed to connect_async: Host and / or port info missing");
		return false;
	}
	printVerbose("Setting Host and Port and connect_async...");

	//************************ connecting to broker *************************************/
	if (connectOrReconnect(broker_settings.ignore_error_first_connect) == false)
	{
		return false;
	}

	///************************ eventually starting the internal mosquitto loop *************************************/
	return exclusiveLoopStart();
}


bool Bridge::tryReconnectMqtt()
{
	ecal_rx_counter = 0;
	mqtt_rx_counter = 0;

	/************************ reconnecting to broker *************************************/
	if ((broker_settings.host.size() == 0) || (broker_settings.port == 0))
	{
	  printError("Failed to reconnect: Host and / or port info missing");
	  return false;
	}
	printVerbose("Reconnecting...");
	if (connectOrReconnect() == false)
	{
	  return false;
	}

	/************************ eventually starting the internal mosquitto loop *************************************/
	return exclusiveLoopStart();
}

// on MQTT Message
void Bridge::on_message(const struct mosquitto_message* message)
{
	if ((is_initialized == false) || (is_connected_to_mqtt_broker == false))
	{
		return;
	}
	// Check if the message that has arrived is a dscriptor message
	// if so check if the descriptor hash is in the table 
	MqttTopic current_topic;
	bool found = false;
	for (auto topic : mqtt2ecal_topics)
	{
		if (topic.mqtt_ecal_type_descriptor == std::string(message->topic))
		{
			current_topic = topic;
			found = true;
		}
	}
	if (found)
	{
		std::string descriptor(static_cast<char*>(message->payload), message->payloadlen);
		auto hash = hasher(descriptor);
		auto current_topic_hash = from_mqtt_desc_hash.find(std::string(message->topic));
		//ToDo: Eliminate duplicated code
		if (current_topic_hash == from_mqtt_desc_hash.end() ||
		   (current_topic_hash->second != hash))
		{
			from_mqtt_desc_hash[message->topic] = hash;
			auto pub_it = ecal_publishers.find(current_topic.ecal_out_topic_name);
			if (pub_it != ecal_publishers.end())
			{
				pub_it->second->SetDescription(descriptor);
			}
		}
	}
	else
	{
		// Find the corresponding eCAL publisher
		MqttTopic ecal_it;
		bool found = false;
		for (auto topic : mqtt2ecal_topics)
		{
			if (topic.mqtt_payload_name == std::string(message->topic))
			{
				ecal_it = topic;
				found = true;
			}
		}
		if (found)
		{
			auto pub_it = ecal_publishers.find(ecal_it.ecal_out_topic_name);
			if (pub_it != ecal_publishers.end())
			{
				mqtt_rx_counter++;
				pub_it->second->Send(message->payload, message->payloadlen);
			}
		}
	}
}

// on MQTT Connect
void Bridge::on_connect(int rc)
{
	printVerbose("on_connect rc: " + std::to_string(rc));
	switch (rc)
	{
	case 0:
	{
		printVerbose("Successfully connected to MQTT Broker");
		// Connect the "normal" mqtt subscriber
		for (auto topic : mqtt2ecal_topics)
		{
			int subscribe_err = subscribe(NULL, topic.mqtt_payload_name.c_str(), topic.qos);
			if (subscribe_err == MOSQ_ERR_SUCCESS)
			{
				printVerbose("Successfully subscribed MQTT topic: " + topic.mqtt_payload_name);
			}
			else
			{
				printError("Failed to subscribe to mqtt topic \"" + topic.mqtt_payload_name + "\"", subscribe_err, MOSQ_STR_ERROR);
				return;
			}
		}
		// Connect the "descriptor" mqtt subscriber
		for (auto topic : mqtt2ecal_topics)
		{
			if (topic.mqtt_ecal_type_descriptor.size() > 0)
			{
				int subscribe_err = subscribe(NULL, topic.mqtt_ecal_type_descriptor.c_str(), topic.qos);
				if (subscribe_err == MOSQ_ERR_SUCCESS)
				{
					printVerbose("Successfully subscribed MQTT topic: " + topic.mqtt_ecal_type_descriptor);
				}
				else
				{
					printError("Failed to subscribe to mqtt topic \"" + topic.mqtt_ecal_type_descriptor + "\"", subscribe_err, MOSQ_STR_ERROR);
					return;
				}
			}
		}
		is_connected_to_mqtt_broker = true;
		break;
	}
	case 1:
	{
		printError("Connection refused while connecting to MQTT Broker (unacceptable protocol version)");
		break;
	}
	case 2:
	{
		printError("Connection refused while connecting to MQTT Broker (identifier rejected)");
		break;
	}
	case 3:
	{
		printError("Connection refused while connecting to MQTT Broker (broker unavailable)");
		break;
	}
	case 4:
	{
		printError("Connection refused while connecting to MQTT Broker (bad user name or password)");
		break;
	}
	case 5:
	{
		printError("Connection refused while connecting to MQTT Broker (not authorised)");
		break;
	}
	default:
	{
		printError("Connection Error unknown: " + std::to_string(rc));
		break;
	}
	}
}
//
//// on MQTT Disconnect
void Bridge::on_disconnect(int rc)
{
	printVerbose("on_disconnect rc: " + std::to_string(rc));
	if (rc == 0)
	{
		printError("disconnected from mqtt broker");
	}
	else
	{
		printError("connection to mqtt broker was closed unexpectedly: " + std::to_string(rc));
	}
	is_connected_to_mqtt_broker = false;
}

void Bridge::on_log(int level, const char* str)
{
	std::string level_str = "[UNKNOWN] ";
	switch (level)
	{
	case MOSQ_LOG_INFO:
		level_str = "[INFO]    ";
		break;
	case MOSQ_LOG_NOTICE:
		level_str = "[NOTICE]  ";
		break;
	case MOSQ_LOG_WARNING:
		level_str = "[WARNING] ";
		break;
	case MOSQ_LOG_ERR:
		level_str = "[ERROR]   ";
		break;
	case MOSQ_LOG_DEBUG:
		level_str = "[DEBUG]   ";
		break;
	default:
		break;
	}
	printVerbose(level_str + std::string(str));
}

// on eCAL Message
void Bridge::ecalMessageReceived(const char* topic_name_, const struct eCAL::SReceiveCallbackData* data_)
{
	if (!is_initialized || !is_connected_to_mqtt_broker) return;
	for (auto topic : ecal2mqtt_topics)
	{
		if (topic.ecal_topic_name == std::string(topic_name_)) {
			publish(NULL, topic.mqtt_out_payload_name.c_str(), data_->size, data_->buf, topic.qos, topic.retain_flag);
		}
	}
	ecal_rx_counter++;
}

bool Bridge::isInitialized() const
{
	return is_initialized;
}

bool Bridge::isConnectedToMqttBroker() const
{
	return is_connected_to_mqtt_broker;
}

int Bridge::getMqttRxCounter() const
{
	return mqtt_rx_counter;
}

int Bridge::getEcalRxCounter() const
{
	return ecal_rx_counter;
}

Bridge::~Bridge(void)
{
	mqtt_desc_thread_active = false;
	is_initialized = false;
	mqtt_desc_thread.join();
	disconnect();
	is_connected_to_mqtt_broker = false;
	loop_stop(true);
	mosqpp::lib_cleanup();
	for (auto const& it_publisher : ecal_publishers)
	{
		delete it_publisher.second;
	}

	for (eCAL::CSubscriber* subscriber : ecal_subscribers)
	{
		delete subscriber;
	}
	eCAL::Finalize();
}

