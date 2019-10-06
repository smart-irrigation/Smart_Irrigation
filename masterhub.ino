#include <NTPClient.h>
#include <ESP8266WiFi.h>                 
#include <WiFiUdp.h>                                  
#include <FirebaseArduino.h>                                               
#include <ArduinoJson.h>
#include <SimpleDHT.h>


#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define firebase_host "temp-nodewise.firebaseio.com"
#define firebase_auth "WrXvNdIzt9lyGonFsevXW94KpijCfHJZ3pN6Tpws"

WiFiUDP ntpUDP;

const int DHT11sensor = 16;                                               //comment this if not having humidity and temperature sensor
SimpleDHT11 dht11(DHT11sensor);                                          //comment this if not having humidity and temperature sensor
int rel[3]={5,4,14};
int startup;

const long utcOffsetInSeconds = 19800;   // off set of current timezone from utc
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);   // for extraction of current date and time from pool.ntp.org with the off set from timezone


//Variables
int i = 0;
int statusCode;
const char* ssid = "text";
const char* passphrase = "text";
String st;
String content;


//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);


void setup() {
  Serial.begin(9600);
  delay(2000);                


  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  //---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");

  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");

  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  WiFi.begin(esid.c_str(), epass.c_str());
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    timeClient.begin();                                                       // timeclient begin for extracting hrs,minutes,seconds
    startup=1;           
    pinMode(rel[0],OUTPUT);
    pinMode(rel[1],OUTPUT);
    pinMode(rel[2],OUTPUT);                          
    Firebase.begin(firebase_host, firebase_auth);                             //connection with firebase is begin here
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");
  
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
  
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
int manual;
int manual_on;

int node_value[3];
int relayon[3];
void loop() {
  if(startup==0)
  {
    startup=1;
    reset_fun();
  }
  manual=Firebase.getInt("/MASTER/manual");
  if(manual==0)
  {
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

    node_value[0]=Firebase.getInt("node_data/allnodedata/node1data");
    node_value[1]=Firebase.getInt("node_data/allnodedata/node2data");
    node_value[2]=Firebase.getInt("node_data/allnodedata/node3data");
    for(int a=0;a<3;a++)
    {
      if(node_value[a]>500)
      {
        relayon[a]=1;
      }
      else{
        relayon[a]=0;
      }
    }
    for(int a=0;a<3;a++)
    {
      if(relayon[a]==1)
      {
         digitalWrite(rel[a],LOW);
      }
      if(relayon[a]==0){
        digitalWrite(rel[a],HIGH);
      }
    }
    int n;
    if(temperature>=30)
      {
        n=Firebase.getInt("/suply_value/crop1/high_n");
      }
      else{
        n=Firebase.getInt("/suply_value/crop1/low_n");
      }
      int temp_node1;
      int temp_node2;
      int temp_node3;
     while(n>=0)
        {
        temp_node1=Firebase.getInt("/node_data/allnodedata/node1data");
        temp_node2=Firebase.getInt("/node_data/allnodedata/node2data");
        temp_node3=Firebase.getInt("/node_data/allnodedata/node3data");
       /* if(temp_node1>500){
          digitalWrite(rel[0],LOW);
        }
        if(temp_node2>500){
          digitalWrite(rel[1],LOW);
        }
        if(temp_node3>500){
          digitalWrite(rel[2],LOW);
        }*/
        n--;
        delay(2000);
        }
        digitalWrite(rel[0],HIGH);
        digitalWrite(rel[1],HIGH);
        digitalWrite(rel[2],HIGH);
        for(int a=0;a<3;a++)
        {
          relayon[a]=0;
        }
        
    delay(2000);
  }
  else{
    manual_on=Firebase.getInt("/MASTER/moter_on_off");
    if(manual_on==1)
    {
      digitalWrite(rel[0], LOW);
      digitalWrite(rel[1], LOW);
      digitalWrite(rel[2], LOW);      

    }
    else{
      digitalWrite(rel[0], HIGH);
      digitalWrite(rel[1], HIGH);
      digitalWrite(rel[2], HIGH);
    }
  }
}


bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);

    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("enter wifi cred", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("over");
}

void createWebServer()
{
 {
    server.on("/", []() {

      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

    });
  } 
}
