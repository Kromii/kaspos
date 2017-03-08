#include "mqtt.h"

void on_message(struct mosquitto *client, void *obj, const struct mosquitto_message *message){

	//Parse id and value from message, call senddata with them
	char *_msg = (char*)message->payload;
	std::string msg(_msg);
	std::string _id;
	std::string _value;

	std::string msgformat;
	int _msgformat;

	msgformat += msg[0];
	msgformat += msg[1];

	try {
		_msgformat = std::stoi(msgformat);
	}
	catch (std::invalid_argument& e){
		if (testmode == 1){
			std::cout << "Invalid message received" << std::endl;
		}
	}

	if (_msgformat == 1) {

		for (int i = 2; i < 5; i++){
			_id += msg[i];
		}

		int id;
		try {
			id = std::stoi(_id);
		}

		catch (std::invalid_argument& e){
			std::cout << "Invalid message received" << std::endl;
			return;
		}

		//If ID of received message isn't valid aka over the amount of sensors;
		if (id >= AoS){
			return;
		}

		for (int i = 5; i < 21; i++){	// Allow value to be 15 digits.
			if (msg[i] == '\0'){
				break;
			}
			_value += msg[i];
		}

		float value;
		try {
			std::replace(_value.begin(), _value.end(), ',', '.');

			value = std::stof(_value);
		}

		catch (std::invalid_argument& e){
			std::cout << "Invalid message received." << std::endl;
			return;
		}


		value = value * modArray[id][0];
		mqttcounter = mqttcounter - 1;
		std::thread t (senddatathread, id, value);
		t.detach();
	}
	fflush(stdout);
}

void mqttlistener(){

        std::cout << "Listener thread initialize" << std::endl;
        struct mosquitto *listener;

        listener = mosquitto_new("rpilistener", true, NULL );
        mosquitto_message_callback_set(listener, on_message);

        mosquitto_connect(listener, "127.0.0.1", 1883, 120);
        mosquitto_subscribe(listener, NULL, "vastaus", 0);

        mosquitto_loop_forever(listener, -1, 1);

}

void mqttsender(){
        std::cout << "Sender thread initialize" << std::endl;
        struct mosquitto *sender;
        sender = mosquitto_new("rpisender", true, NULL );
        mosquitto_connect(sender, "127.0.0.1", 1883, 120);
        while (true){
		//If requestflag is 1, request data and flag down
		if (rflag == 1){
			rflag = 0;
			int id = globalid;
			std::string _msg;
			char msg[20];
			// MQTT request message forms:
			int msgform = busArray[id][3];
			//Message formate 1 start;
			if (msgform == 1){

				if (msgform < 10){
					_msg += "0";
				}
				_msg += std::to_string(msgform);

				if (id > 9 && id < 100){			//sensor MAIN ID number. 3 digits
					_msg += "0";
				}

				if (id < 10){
					_msg += "00";
				}

				_msg += std::to_string(id);

				if (busArray[id][0] < 10){			// Arduino ID number from config. 2 digit
					_msg += std::to_string(0);
				}

				_msg += std::to_string(busArray[id][0]);

				/*if (busArray[id][1] > 9 && busArray[id][1] < 100){
					_msg += "0";
				}*/

				if (busArray[id][1] < 10){			// Arduino sensor ID number from config, 2digit
					_msg += "0";
				}

				_msg += std::to_string(busArray[id][1]);

			} // message format 1 end

			//message format 2 start
			else if (msgform == 2){
				//PLACEHOLDER EXAMPLE
			}
			strcpy(msg, _msg.c_str());

			mosquitto_publish(sender, NULL, "kysely", strlen(msg),msg, 0, false);
			if (testmode == 1){
				std::cout << "MQTT request sent, " << mqttcounter << std::endl;
			}
			mqttcounter = mqttcounter + 1; // request-reply counter ++
			mosquitto_loop(sender, -1, 1);
                }
        }
}
