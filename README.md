Project: IoT Waste Management

![waste_management](https://user-images.githubusercontent.com/78800629/207486314-2bf965a8-c95c-44e7-95de-c38345326658.png)

	Background

This is part of an assignment in **IoT and cloud Services** where I've used a distance sensor for reading current waste level in my household bin. I've since a year back been studying IoT, and by now grown quite tired of playing around with the temp- and humid sensor, and thought this was a brilliant idea. However, figure out a use case turned out to be way more difficult than I first realized given the circumstances.

	Use Case

Closing my eyes while throwing the waste and let an interface of some sort remind me when it's time to take action is neat but not really much of a use case. I had to use a little bit of imagination in this one why I've built my architecture trough the lenses of a Waste Management Company. I'm going to refer to this entity as *WMC* in the documentation.

The idea is simple: devices will publish data to the cloud. Vessels that's full will trigger the cloud to send a SMS about which vessel, and where it's located, to a garbage collector. We're required to implement an API in this assignment, and after a class mate told me about a town in southern Sweden in which vessels being dealt with more frequently as the temperature rises I've utilized a temp API for something similar in this project. 

	(1) Hardware

**MDevice**, "the device", is an ESP8266 (Wemos D1 mini) in circuit with a distance sensor (HC-SR05). The device is placed over a vessel (my trash can) in a garbage room (the area under my sink) within a given district (my apartment).

	(2) Software and API

Most logic in place revolves around the *sensor_value*, *vessel_state* and *update_event* variables whereas *sensor_value* will determine current *vessel_state*. If a change occurs, the *update_event* variable will be set, sent to the cloud and invoke rules that going to trigger the "the core" to execute some actions. There are 3 of these vessel states:
|  state|definition  |
|--|--|
|  green| vessel empty  |
|  yellow| vessel half full  |
|  red| vessel full  |

For example; a sensor_value between 35-50 cm might be conditionally attached to state green, 20 - 34 cm to yellow and 0-19 cm red. I wrote *might* because these threshold values are not flashed constants but configurated from the cloud. I've implemented this feature for "long term" purposes. Instead of making, in this regard, the thing kinda obsolete for every vessel change I thought it might be handy to just publish - "update" - new threshold values to all devices concerned through MQTT commands from the cloud.
    
As API goes I'm utilizing temperatur.nu. The device will request temp data every four hour from a sensor close to where I'm living (the "district"), a value rest of the logic will take in consideration. If the reading is > 30, we're dealing with temperatures too high for taking "state yellow" into account. I.e: dispatch requests will occur more often.

Yes, it's probably quite a Quasimodo solution in this context to read an api from the device instead of (input anything else, why not the cloud...) but I just didn't manage to make any API calls from there (?). My goto was to use lambdas to settle this matter but the platform refused all basic libs (Python) I'm used to. First time I'm working with cloud services, sure, but such easy task shouldn't be a problem.

The flash itself is done from the Arduino-IDE using ESP8266 as target from the board manager.

![check_states](https://user-images.githubusercontent.com/78800629/207491443-39f6edac-b5e6-4a5d-8e04-bfe902c1eb0f.png)

	(3) Device Gateway

The entry point for connected devices where checks and verifications being done before a (hopefully) long lived connection is established (MQTT). However, unlike the device under my sink, *WMC* might oversee thousands of devices and probably think twice before copy/pasting and burn this kind of sensitive information straight to the metal. More on this later.

	(4) IoT Policies

Services for establishing MQTT connection with "the core's" message broker. Specifies which resources and actions being allowed for a device. These policies governs the device(s) shadows as well, something I've illustrated as a concept in my diagram but not implemented. Not yet at least.

A shadow is a digital representation of a device and will store, query, actions and desired states until a device that's - for example - is dead going online again. When it comes to config, "updating",  threshold values on my device(s), there's far from any deacent interface in place why I've used *AWS MQTT client test* tool for this purpose.

![config](https://user-images.githubusercontent.com/78800629/207476023-9257b320-d652-45a7-b293-bc4f4c063e0b.png)

    (5) IoT Topics
    
The MQTT brooker identifies messages from the topics they're sent and received from. As for "WMC", I've implemented a region/district/garbage_room/vessel/device_id/"pub/sub" hierachy.

    (6) IoT Rules

Service that's filtering and take actions based on content sent from a device. The filtering is done by parsing incoming messages using SQL syntax. You enter this snippet when creating (or updating) a rule. I've created rules that forwarding every message that has the attribute *update_event* set to a dynamoDB table (warm path) and a S3 bucket (cold path).

I call these *event messages* and they're the only ones being stored in databases. As for now, the device is flashed for publishing data every second and to store ALL of these messages - even if so be a S3 - felt a bit excessive.

This binary value of *update_event* is calculated, by old habit, in the source code of my device but I'm quite sure a service like *IoT events* had been handy in this matter. From the documentation I've even (rather of course) noticed there's logic available for utilizing a state machine.

![put_dynamo](https://user-images.githubusercontent.com/78800629/207476242-c268f13d-ad7b-4bfa-9dd0-a8fcf1f43ea7.png)

	(7) AWS KMS och IAM

Unlike when I'm flashing "secrets" to the prototype, *AWS Key Management Service (AWS KMS)* would suit an upscaled - real - interpretation of my architecture better. This service centralizes security management while - among other - taking care of validations, pairing identities with certificates/keys digitally and encrypting/decrypting data flows.

*AWS Identity and Access Management*, IAM, makes it possible to manage which services and resources users/employee's have access to. There's, however, as we know only been one device connected during my experiments and even though I've a large company in mind, one user except root felt more than enough in this project.

![employee](https://user-images.githubusercontent.com/78800629/207476308-fa8f7191-3c03-43c9-8fcd-c222820bdf50.png)

	(8) Cold Path (9) Warm Path
   
My original plan for these guys didn't work out as expected. In short, every *event message* from this day and until the end of time would had been thrown into the bucket while DynamoDB's main purpose was to visualize ongoing, short term activity using "quicksight* for the imaginary employees in the office to enjoy. Sure, data is written to both identities but that's about it. Regional settings doesn't allow me to visualize it (or I've failed to understand how).

![dynamodb](https://user-images.githubusercontent.com/78800629/207492224-00c3df54-e837-42c6-981b-3d5a1a66fbe9.png)

	(10) Hot Path
    
While the office people haven't anything to do except updating devices all day using the *AWS MQTT client test* I've at least functionality - in some sense a visualization in its own right - in place to alarm waste collectors when a vessel is ready to be emptied.

I'm utilizing *Amazon SNS notification service* for this task. I've created a rule where messages that just changed vessel state from yellow to red tells ASNS to send me/made up garbage collectors the happy news.

In a real case scenario, these text messages would preferley be sent to the closest collector from where the vessel (and the device) is located. That's for next time.

![sms_update](https://user-images.githubusercontent.com/78800629/207476506-ad2a5576-f1ce-43cc-81f7-1f00a32e5f1f.png)
