**Overview**
![Uploading WhatsApp Image 2024-11-28 at 23.14.09_7fcba4cf.jpg…]()
This project implements a Smart Street Light System designed to optimize energy consumption through automated and intelligent control of streetlights. The system adjusts lighting based on environmental conditions and provides real-time monitoring of lamp status through a web-based user interface powered by Node-RED.

**System Architecture** 

The system consists of two main boxes, each equipped with sensors and an ESP32 controller. These nodes work together to control the brightness of streetlights and monitor their operational status. To reduce the usage or amount of the Light Sensor, Data from 1 sensor is exchanged using ESP-Now peer to peer  connection. When the data reaches the final node, it will be sent via MQTT to a Node-RED dashboard, allowing users to monitor the system remotely.

