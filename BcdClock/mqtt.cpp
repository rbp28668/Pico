#include "mqtt.hpp"

MqttClient::MqttClient(const char *clientName, const ip_addr_t *ip_addr, u16_t port)
{

    /* Setup an empty client info structure */
    memset(&clientInfo, 0, sizeof(clientInfo));

    /* Minimal amount of information required is client identifier, so set it here */
    clientInfo.client_id = clientName;
    // Note can also set username/pwd,  qos, last will topic.

    client = mqtt_client_new();

    // Make sure callbacks are set up for incoming publish and data
    mqtt_set_inpub_callback(client,
                            incomingPublishCallback,
                            incomingDataCallback,
                            this);

    /* Initiate client and connect to server, if this fails immediately an error code is returned
       otherwise mqtt_connection_cb will be called with connection result after attempting
       to establish a connection with the server.
       For now MQTT version 3.1.1 is always used */

    err_t err = mqtt_client_connect(client, ip_addr, port, &connectionCallback, this, &clientInfo);

    /* For now just print the result code if something goes wrong */
    if (err != ERR_OK)
    {
        printf("mqtt_connect return %d\n", err);
    }
}

MqttClient::~MqttClient()
{
    if (mqtt_client_is_connected(client))
    {
        mqtt_disconnect(client);
    }
    mqtt_client_free(client);
    client = nullptr;
}

bool MqttClient::isConnected()
{
    return mqtt_client_is_connected(client) == 1;
}

void MqttClient::connectionCallback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    MqttClient *mqtt = static_cast<MqttClient *>(arg);
    mqtt->onConnection(client, status);
}

void MqttClient::onConnection(mqtt_client_t *client, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("Connected\n");
    }
}

// Incoming publish data callback function. Called when data arrives to a subscribed topic
void MqttClient::incomingDataCallback(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    Topic *t = static_cast<Topic *>(arg);
    t->OnData(data, len, flags);
}

// Incoming publish function. Called when an incoming publish arrives to a subscribed topic
void MqttClient::incomingPublishCallback(void *arg, const char *topic, u32_t tot_len)
{
    Topic *t = static_cast<Topic *>(arg);
    t->OnPublish(topic, tot_len);
}

// Request callback. Called when a subscribe, unsubscribe or publish request has completed
void MqttClient::requestCallback(void *arg, err_t err)
{
    Topic *t = static_cast<Topic *>(arg);
    t->OnRequest(err);
}

MqttClient::Topic *MqttClient::publish(const char *topic, const char *message, u8_t qos, u8_t retain)
{
    Topic *t = new Topic(this, topic);
    t->publish(message, qos, retain);
    return t;
}

MqttClient::Topic *MqttClient::subscribe(const char *topic, Receiver *receiver, u8_t qos)
{
    Topic *t = new Topic(this, topic, receiver);
    t->subscribe(qos);
    return t;
}

MqttClient::Topic::Topic(MqttClient *client, const char *name)
    : client(client), topicName(name)
{
}


MqttClient::Topic::Topic(MqttClient *client, const char *name, Receiver *receiver)
    : client(client), topicName(name), receiver(receiver)
{
}

void MqttClient::Topic::subscribe(u8_t qos)
{

    err = mqtt_sub_unsub(client->client, topicName, qos, MqttClient::requestCallback, this, 1);
    if (err == ERR_OK)
        complete = false;
}

void MqttClient::Topic::publish(const char *message, u8_t qos, u8_t retain)
{
    err = mqtt_publish(client->client, topicName, message, strlen(message), qos, retain, MqttClient::requestCallback, this);
    if (err == ERR_OK)
        complete = false;
}

void MqttClient::Topic::OnData(const u8_t *data, u16_t len, u8_t flags)
{
    receiver->OnData(data, len, flags);
}
void MqttClient::Topic::OnPublish(const char *topic, u32_t tot_len)
{
    receiver->OnPublish(topic, tot_len);
}
void MqttClient::Topic::OnRequest(err_t err)
{
    this->err = err;
    complete = true;
    receiver->OnRequest(err);
}