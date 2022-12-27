#include "mbed.h"
#include "bbcar.h"

#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

Ticker servo_ticker;
Ticker servo_feedback_ticker;

PwmIn servo0_f(D9), servo1_f(D10);      //servo
PwmOut servo0_c(D11), servo1_c(D12);
BBCar car(servo0_c, servo0_f, servo1_c, servo1_f, servo_ticker, servo_feedback_ticker);

BusInOut qti_pin(D4,D5,D6,D7);      //qti 

DigitalInOut pin8(D8);      //ping 
parallax_laserping  ping1(pin8);

parallax_qti qti1(qti_pin);
int pattern;
int ans=0;
int deg=0;
int checkpoint=0;

//mqtt
WiFiInterface *wifi;
InterruptIn btn2(BUTTON1);
// volatile int message_num = 0;
// volatile int arrivedcount = 0;
volatile bool closed = false;
const char* topic = "Mbed";
Thread mqtt_thread(osPriorityHigh);
EventQueue mqtt_queue;

//mqqt

void publish_message(MQTT::Client<MQTTNetwork, Countdown>* client) {
    MQTT::Message message;
}

void messageArrived(MQTT::MessageData& md) {
    MQTT::Message &message = md.message;
}

void close_mqtt() {
    closed = true;
}

MQTT::Message message;
char buf[100];

void sendmsg(){
    sprintf(buf, "%d", checkpoint);
    printf("Puslish message: %s\n", buf);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
}


void checking_object(){
    car.turn(-50, -0.1);  // turn left 

    printf("scanning for object\n" );
    while(1) {
        printf("Ping = %f\r\n", (double)ping1);
      if((double)ping1>50);
      else {
        printf("object on the left has a distance of = %f\n", (double)ping1);
        car.stop();
        break;
      }
      ThisThread::sleep_for(10ms);
   }
    car.turn(-50, 0.1);  // turn right 
    ThisThread::sleep_for(2s);

    while(1) {
       printf("Ping = %f\r\n", (double)ping1);
      if((double)ping1>50) ;
      else {
        printf("object on the right has a distance of = %f\n", (double)ping1);
        //printf("%d \n" , car.servo1.angle);
        ans = ((sin(car.servo1.angle/2)*(double)ping1));
        printf("distance between two object is approximately %d you can pass forward\n", abs(ans*2));
        car.stop();
        break;  //break this while
      }
      ThisThread::sleep_for(10ms);
   }

    printf("turn left 0.1 \n");
    car.turn(-50, -0.1);  // turn left back to the same pos
    ThisThread::sleep_for(1500ms);

   return;
}

void object_infront(){
    car.goStraight(-50); //moveforward 
    printf("move forward \n");
   while(1) {
        pattern = (int)qti1;
        printf("%d\n",pattern);
      printf("Laser Ping = %lf\r\n", (double)ping1); //Laser Ping's distance
      if((double)ping1>5);
      else {
         car.stop();            //stop
         ThisThread::sleep_for(500ms); 
         printf("obeject in front move backward slowly\n");
        car.goStraight(50);    //move backward
        ThisThread::sleep_for(500ms);
      }

      if (pattern == 0b1111){
        car.goStraight(-50); //moveforward 
        ThisThread::sleep_for(200ms);
        break;
      }
      ThisThread::sleep_for(10ms);
   }
   return;
}

int main() {
    //connect wifi first//
    wifi = WiFiInterface::get_default_instance();
    printf("\nConnecting to %s...\r\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);

    NetworkInterface* net = wifi;
    MQTTNetwork mqttNetwork(net);
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    const char* host = "172.20.10.2";
   
    const int port=1883;
    printf("Connecting to TCP network...\r\n");
    printf("address is %s/%d\r\n", host, port);

    int rc = mqttNetwork.connect(host, port);//(host, 1883);
    printf("Successfully connected!\r\n");
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "Mbed";

    if ((rc = client.connect(data)) != 0){
            printf("Fail to connect MQTT\r\n");
    }
    if (client.subscribe(topic, MQTT::QOS0, messageArrived) != 0){
            printf("Fail to subscribe\r\n");
    }
    // end

   printf("Start\n");
   //object_infront();

   car.goStraight(-50);
   while(1) {
      car.checkDistance(1);

      pattern = (int)qti1;
      printf("%d\n",pattern);
      
      if (pattern == 0b1000){car.turn(-50, 0.1);}  // 8  right  
      else if (pattern == 0b1100){car.turn(-50, 0.5);}
      else if (pattern == 0b0100){car.turn(-50, 0.7);}  
      else if (pattern == 0b0110){car.goStraight(-50);}  // 6 forward
      else if (pattern == 0b0010){car.turn(-50, -0.7);}//2 left  alittle
      else if (pattern == 0b0011){car.turn(-50, -0.5);}
      else if (pattern == 0b0001){car.turn(-50, -0.1);}
      else if (pattern == 0b0000){car.goStraight(50);}///0  go backward
      else if (pattern == 0b0111 && checkpoint == 3){   // 7 
                //printf("checking object\n");
                checkpoint = 4;
                sendmsg();
                sprintf(buf, "checking object\n");
                printf(buf);
                rc = client.publish(topic, message);
                checking_object();

                sprintf(buf, "distance between two object is approximately %d \n", abs(ans*2));
                printf(buf);
                message.payloadlen = strlen(buf)+1;
                rc = client.publish(topic, message);            
            } //7
      else if (pattern == 0b1110){car.turn(-50, -0.7);} //14
      else if (pattern == 0b1101){car.turn(-50,-0.7);}//13 just incase
      else {car.goStraight(-50);}       // else just go forward


      if (pattern == 0b1111){
         car.stop(); 
         printf("car stop\n");
         ThisThread::sleep_for(2s);
         checkpoint += 1;       
         printf("check point = %d\n",checkpoint);
         
         car.goStraight(-50);
         printf("continue moving\n");
         
        if (checkpoint == 1){ //for exiting the circle
            //printf("checkpoint 1 turn left\n");  
            sendmsg();
            rc = client.publish(topic, message);
            sprintf(buf, "checkpoint 1 turn left\n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);

            car.turn(-50, -0.1); //car turn left to exit the circle
            ThisThread::sleep_for(2s);

            if (pattern == 6){
                car.goStraight(-50);
            }   
        }
        if (checkpoint == 2){
            car.goStraight(-100);
            ThisThread::sleep_for(1s);
            sendmsg();
            rc = client.publish(topic, message);
            sprintf(buf, "straight forward road \n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);
        }

        if (checkpoint == 3){
            //printf("normal speed\n"); 
            car.goStraight(-50);
            ThisThread::sleep_for(1s);
            sendmsg();
            rc = client.publish(topic, message);
            sprintf(buf, "turning soon\n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);
        }
         
        //  if (checkpoint == 4){
        //     printf("checking object\n");
        //     checking_object();
        //  }

        if (checkpoint == 5){
            sendmsg();  
            rc = client.publish(topic, message);
            object_infront();

            sprintf(buf, "object infront move backward\n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);
        }

        if(checkpoint == 6){
            car.turn(-50, 0.1); //turn rightttt to go the other way
            ThisThread::sleep_for(2s);
            sendmsg();
            rc = client.publish(topic, message);
            sprintf(buf, "turn right since there an object infront\n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);

            if (pattern == 6){
                car.goStraight(-50);
            }
        }

        if (checkpoint == 7){
            sendmsg();
            rc = client.publish(topic, message);

            sprintf(buf, "curve road infront\n");
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);

            car.goStraight(-50);
            ThisThread::sleep_for(1s);
        }
 
        if (checkpoint == 8){
            car.stop(); 
            sendmsg();
            rc = client.publish(topic, message);
            //printf("distance = %f\n", abs(((car.servo0.angle)*6.5*3.14/360)));
            sprintf(buf, "the car have reach it destination, approximate distance travel= %f \n", abs(((car.servo0.angle)*6.5*3.14/360)));
            printf(buf);
            message.payloadlen = strlen(buf)+1;
            rc = client.publish(topic, message);
            break;
        }
    }

    ThisThread::sleep_for(100ms);
   }

    printf("Ready to close MQTT Network......\n");

    if ((rc = client.unsubscribe(topic)) != 0) {
            printf("Failed: rc from unsubscribe was %d\n", rc);
    }
    if ((rc = client.disconnect()) != 0) {
    printf("Failed: rc from disconnect was %d\n", rc);
    }

    mqttNetwork.disconnect();
    printf("Successfully closed!\n");

    return 0;
}