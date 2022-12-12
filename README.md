Project: Waste Management


![tlted_latest drawio](https://user-images.githubusercontent.com/78800629/206994519-94a6e52e-b879-4147-b2f6-d3783c4bdd7e.png)

	Background
	
This is an assignment in IoT and cloud services where I've used a distance sensor for publishing data about it's state into an architecture built out of some of all services AWS has to offer.

    Use Case
    
This part turned out to be quite difficult. Close my eyes and outsource the looking part to a led device/dashboard of some sort to tell if it's time to empty the vessel or not is cool but maybe not attached with the most solid use case. Because it's required to implement an API and utilize fetched data, somehow and somewhere, for visualization, I've taken a imaginary approach where I'm a Waste Management Company.

After a class mate told me how Waste Management in Trelleborg dispatch vessels more frequent as the temperature rises I've utilized a temp API for something similar in this project. Yes, it had probably been easier to just connect a temp sensor in circuit with my distance sensor for this but it does job! 

	(1) Hardware

**MDevice**, from here and throughout the documentation refered to as "the thing", "prototype" or "the device", is an ESP8266 (Wemos D1 mini) in circuit with a distance sensor (HC-SR05). The device is placed over a given vessel (my trash can) in a garbage room (the area under my sink), one of many within an imaginary district (my apartment).

<< Picture by deadline >>

	(2) Software

My architecture is built from the device up to the cloud and into the *AWS IoT Core*. For in-depth explanation, go ahead and read the source code, but its core concept involves the variables "sensor_value* and *vessel_state* whereas *sensor_value* will determine current *vessel_state*. If a change occurs, the *update_event* variable will be set and trigger a *rule* which triggers the "the core" to insert this sample into *DynamoDB*. There are 3 vessel states:
|State | Definition |
|--|--|
|  green|vessel empty|
|  yellow|vessel half full|
|  red|vessel full|

For example; a *sensor_value* between 35-50 cm, *might* be conditionally attached to state green, 20 - 34 cm to yellow and 0-19 cm red. I wrote *might* because these threshold values are not constants flashed into the **MDevice** but configurated from the cloud. I've implemented this feature for "long term" purposes.

After all, vessels come in different sizes and shapes while new standards come and go. Instead of making, in this regard, the thing obsolete for every change I thought it might be handy to just publish, "update", new threshold values for all devices concerned through MQTT commands from the cloud. "Ish".

<< Picture by deadline >>

    (3) Security

Unlike the phony device under my sink, a real Waste Management Company might oversee thousands of devices and probably think twice before copy/pasting this kind of sensitive information and burn it to the metal. Guess *AWS Key Management Service (AWS KMS)* might suit this second thought well; a service that centralizes security management while - among others - pairing identities with certificates and keys digitally and encrypt- and decrypts data flows.

<< Picture by deadline >>
	
    (4) IoT Policies

Configurations for establishing MQTT connection with "the core's" message broker. Specifies which resources and actions being allowed for a device. These policies governs the device(s) shadows as well, something I've illustrated as a concept in my diagram but not implemented myself yet. A shadow is a digital representation of a device and will store, query, ongoing and desired actions until a device that's, for example offline, going online again.

I've let MDevice be able to publish and subscribe to the cloud. This will ... [documentation ongoing]
<Picture by deadline>

    (5) IoT Topics
