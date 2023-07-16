## LoRa RHMesh

> This repository is a place in which I include all projects for LoRa RHMesh development for my thesis. I provide all projects within the `projects` folder. All development are made by myself, yet it is made based upon many different projects scattered all over the internet.
>
> I develop mainly on an ESP32 board, connected with a RFM95 LoRa Chip. It transmits on 915 MHz. I develop on 2 environments, Arduino IDE, Platform IO VSCode Plugin.
>
> Feel free to use them, and contact me if you will!

This example sketch shows how to create a simple addressed, routed reliable messaging client with the RHMesh class. It is designed to work with the other examples rf95_mesh_server

### Getting Started
- Open VSCode, install PlatformIO
- Make sure you have the wiring correct!
- Change the current nodes ID and target node ID (to send message to) using the line
```
#define SELF_ADDRESS NODE3_ADDRESS
#define TARGET_ADDRESS FINAL_ADDRESS
```
- You can simulate other network topologies by setting the `RH_TEST_NETWORK` define your topology in RHRouter.cpp. read more in [Forced Topology section](#forced-topology)

### Wiring
```
[RFM95] ------------- [ESP32]
RESET  -------------- GPIO14
NSS/CS -------------- GPIO5
SCK    -------------- GPIO18
MOSI   -------------- GPIO23
MISO   -------------- GPIO19
DIO0   -------------- GPIO2

3.3V   -------------- 3.3V
GND    -------------- GND
```

if you have a different wiring scheme, don't forget to change these lines in main.cpp
```
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2
```

Change to 915.0, 434.0 or other frequency, must match LoRa chip/RX's freq!
```
#define RF95_FREQ 915.0
```

By using Mesh, it has much greater memory requirements than just RH or RHRouter, and you may need to limit the max message length (characters) to prevent wierd crashes. Though you can change and experiment with message lengths by changing this
```
#define RH_MESH_MAX_MESSAGE_LEN 50
```

### Topology
This example of 4 node topology, in which the FINAL_ADDRESS node is expected to be the last node, simulating a common network in which this last node would be a border node connected to the internet, collecting messages from other nodes during it's lifetime. whilst node 1-3 would commonly send sensor data, and one could be an intermediary node for another. Feel free to make a totally different addressing
```
#define NODE1_ADDRESS 1
#define NODE2_ADDRESS 2
#define NODE3_ADDRESS 3
#define FINAL_ADDRESS 4
```

You can actively change the current nodes behaviour by changing this line. Make sure you change it for every different node!
```
#define SELF_ADDRESS NODE3_ADDRESS
#define TARGET_ADDRESS FINAL_ADDRESS
```

<a name="forced-topology">
### Forced Topology

Even though this very project runs on RHMesh, which would expect the user to have a fully dynamic and fluid topology, you can force the routes/topology. it requires a little bit of hardcoding, you can inspect the code in RHRouter.cpp (line 223-263). It already has some premade topology examples that forces routing a certain way (it does this by dropping/not processing messages that does not comply the path), and the macro "RH_TEST_NETWORK" needs to be defined (before calling #include "RHMesh.h") to activate this forced topology. you can ofcourse add your own code that resembles your desired topology.
```
...
#ifdef RH_TEST_NETWORK
	if (
#if RH_TEST_NETWORK==1
	    // This network looks like 1-2-3-4
	       (_thisAddress == 1 && _from == 2)
	    || (_thisAddress == 2 && (_from == 1 || _from == 3))
	    || (_thisAddress == 3 && (_from == 2 || _from == 4))
	    || (_thisAddress == 4 && _from == 3)
	    
#elif RH_TEST_NETWORK==2
	       // This network looks like 1-2-4
	       //                         | | |
	       //                         --3--
	       (_thisAddress == 1 && (_from == 2 || _from == 3))
	    ||  _thisAddress == 2
	    ||  _thisAddress == 3
	    || (_thisAddress == 4 && (_from == 2 || _from == 3))
...
```