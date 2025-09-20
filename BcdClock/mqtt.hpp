#ifndef MQTT_CLIENT_HPP
#define MQTT_CLIENT_HPP

extern "C"
{
#include <cyw43.h>
#include "lwip/apps/mqtt.h"
}

class MqttClient
{

public:

    /// @brief Client needs to provide an implementation of this interface when
    /// subscribing to a topic.
    class Receiver
    {
    public:
        virtual void OnData(const u8_t *data, u16_t len, u8_t flags) = 0;
        virtual void OnPublish(const char *topic, u32_t tot_len) = 0;
        virtual void OnRequest(err_t err) = 0;
    };

    
    class Topic
    {
        friend class MqttClient;

        MqttClient *client;
        const char *topicName;
        Receiver* receiver;

        err_t err;
        bool complete;


       Topic(MqttClient *client, const char *name);
        Topic(MqttClient *client, const char *name, Receiver* receiver);
        void subscribe(u8_t qos = 1);
        void publish(const char *message, u8_t qos = 1, u8_t retain = 0);
        void OnData(const u8_t *data, u16_t len, u8_t flags);
        void OnPublish(const char *topic, u32_t tot_len);
        void OnRequest(err_t err);
    };

private:
    mqtt_client_t *client;
    mqtt_connect_client_info_t clientInfo;

    // Mqtt connection status callback. Called when client has connected to the server after initiating a mqtt connection
    // attempt by calling mqtt_connect() or when connection is closed by server or an error
    static void connectionCallback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
    void onConnection(mqtt_client_t *client, mqtt_connection_status_t status);

    // Incoming publish data callback function. Called when data arrives to a subscribed topic
    // Arg will point to Topic object
    static void incomingDataCallback(void *arg, const u8_t *data, u16_t len, u8_t flags);

    // Incoming publish function. Called when an incoming publish arrives to a subscribed topic
    // Arg will point to Topic object.
    static void incomingPublishCallback(void *arg, const char *topic, u32_t tot_len);

    // Request callback. Called when a subscribe, unsubscribe or publish request has completed
    // Arg will point to Topic object.
    static void requestCallback(void *arg, err_t err);

public:
    MqttClient(const char *clientName, const ip_addr_t *ip_addr, u16_t port = MQTT_PORT);
    ~MqttClient();

    bool isConnected();

    Topic* publish(const char* topic, const char *message, u8_t qos = 1, u8_t retain = 0);
    Topic* subscribe(const char* topic, Receiver* receiver, u8_t qos = 1);
};

#endif
