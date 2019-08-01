#include <NTPClient.h>
#include <ESP8266WiFi.h>                 
#include <WiFiUdp.h>                                  
#include <FirebaseArduino.h>                                               
#include <ArduinoJson.h>
#include <SimpleDHT.h>

#define firebase_host "temp-nodewise.firebaseio.com"
#define firebase_auth "WrXvNdIzt9lyGonFsevXW94KpijCfHJZ3pN6Tpws"
#define WIFI_SSID "dhrumil"                                             //wifi name for node mcu to connect with 
#define WIFI_PASSWORD "9825381851"

WiFiUDP ntpUDP;

const int DHT11sensor = 16;                                               //comment this if not having humidity and temperature sensor
SimpleDHT11 dht11(DHT11sensor);                                          //comment this if not having humidity and temperature sensor

const int relay1 = 5;                                                    // 
int startup;

const long utcOffsetInSeconds = 19800;   // off set of current timezone from utc
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);   // for extraction of current date and time from pool.ntp.org with the off set from timezone


void setup() {
  Serial.begin(9600);
  delay(2000);                
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                     //node mcu connects with wifi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  timeClient.begin();                                                       // timeclient begin for extracting hrs,minutes,seconds
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP Address is : ");
  Serial.println(WiFi.localIP());
  startup=0;           
  pinMode(relay1,OUTPUT);                          
  Firebase.begin(firebase_host, firebase_auth);                             //connection with firebase is begin here

}
void reset_fun()
{
  Serial.println("Resetting master.....");
  ESP.restart();
}

String formattedDate;                                                     
String dayStamp;
int value_node1;

byte temperature = 0;                                                       //declaration for temperature        //comment this if not having humidity and temperature sensor
byte humidity = 0;                                                          //declaration for humidity           //comment this if not having humidity and temperature sensor
String dht_combine;

const int index_moisture_dry= 500;                                          // reading for the dry state
const int index_moisture_notdry=375;                                        //reading for the moist state
  
int master_reset;

void loop() {
  if(startup==0)
  {
    startup=1;
    reset_fun();
  }
    timeClient.update();
    formattedDate =(String)timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    Serial.println(dayStamp);
    
    String hrs=(String)timeClient.getHours();
    String min1=(String)timeClient.getMinutes();
    String time1=(String)timeClient.getHours()+":"+(String)timeClient.getMinutes()+":"+(String)timeClient.getSeconds();
    Serial.println(time1);
    master_reset=Firebase.getInt("RESET_NODES/MASTER_RESET");
    Serial.println(master_reset);
    
    if(master_reset==1)
    {
      Firebase.setInt("RESET_NODES/MASTER_RESET",0);
      reset_fun();
    }
    //block of temperature and humidity starts here

    int err = SimpleDHTErrSuccess;                                                        //comment this if not having humidity and temperature sensor
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)         //comment this if not having humidity and temperature sensor
    {                                                                                     
        Serial.print("Read DHT11 failed, err="); Serial.println(err);                     //comment this if not having humidity and temperature sensor
        delay(1000);                                                                      //comment this if not having humidity and temperature sensor
        return;                                                                           //comment this if not having humidity and temperature sensor
    }
    else{                                                                                 //comment this if not having humidity and temperature sensor  
          Serial.print((int)temperature); Serial.print(" *C, ");                          //comment this if not having humidity and temperature sensor
          Serial.print((int)humidity); Serial.println(" H");                              //comment this if not having humidity and temperature sensor
          dht_combine=(String)temperature +"*C   :   " + (String)humidity;
    }
    Firebase.setString("/Master/"+dayStamp+"/dhtvalues/"+time1, dht_combine);                           //comment this if not having humidity and temperature sensor
    if(Firebase.failed())                                                                 //comment this if not having humidity and temperature sensor
    {
      Serial.println("Failed");                                                           //comment this if not having humidity and temperature sensor
    }
    else                                                                                  //comment this if not having humidity and temperature sensor
    {
      Serial.println("Success");                                                          //comment this if not having humidity and temperature sensor
    }                                                                                     //comment this if not having humidity and temperature sensor

    //block of temperature and humidity ends here

    int value_node1=Firebase.getInt("/node1/Moisture/");
    Serial.println(value_node1);

     if(value_node1 >= index_moisture_dry)
    {
      int temp=value_node1;
      
      int n;
      if(temperature>=30)
      {
        n=Firebase.getInt("/suply_value/crop1/high_n");
      }
      else{
        n=Firebase.getInt("/suply_value/crop1/low_n");
      }
      time1=time1+"ON";
      /*Firebase.setString("/logs/"+dayStamp+"/log_value", time1);
      if(Firebase.failed())
      {
        Serial.println("log not created!!!");
      }
      else{
        Serial.println("log created!!!");
      }*/
      while(n>=0)
      {
        digitalWrite(relay1, LOW);
        n--;
        delay(4000);
      }
      digitalWrite(relay1, HIGH);
    }
    
    
    delay(2000);
}
