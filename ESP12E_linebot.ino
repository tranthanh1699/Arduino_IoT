/* 
NodeMCU (ESP12E)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2018-10-16 20:00
https://www.facebook.com/francefu
*/

#include <ESP8266WiFi.h>     //ESP12E

const char* ssid     = "xxxxx";   //your network SSID
const char* password = "xxxxx";   //your network password

void setup()
{
  Serial.begin(115200);
  delay(10);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(1000);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
      delay(500);
      if ((StartTime+10000) < millis()) break;
  } 

  if (WiFi.status() == WL_CONNECTED)
  {
    pinMode(2, OUTPUT);
    for (int i=0;i<5;i++)
    {
      digitalWrite(2,LOW);
      delay(100);
      digitalWrite(2,HIGH);
      delay(100);
    }
      
    Serial.println();
    Serial.println("STAIP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();     
  }  
  else
     Serial.println("Unable to connect!"); 

  if (WiFi.status() == WL_CONNECTED)
  {
    // IFTTT (this: Webhooks, that: Webhooks) -> Google Apps Script > Line Bot
    // Google Apps Script: https://github.com/fustyles/webduino/blob/gs/linebot_push_message.gs

    String domain="maker.ifttt.com";
    String key="dwGDpfZMocgp179E5zpZwuxxx";
    String event="",value1="",value2="",value3="";
    String request="";
    
    // text
    event="linebot_text";
    value1=WiFi.localIP().toString();
    request = "/trigger/"+event+"/with/key/"+key+"?value1="+value1;
    Serial.println(tcp(domain,request,80,0));
    Serial.println();

    delay(5000);

    // sticker
    event="linebot_sticker";
    value1="1";
    value2="2";
    request = "/trigger/"+event+"/with/key/"+key+"?value1="+value1+"&value2="+value2;
    Serial.println(tcp(domain,request,80,0));
    Serial.println();

    delay(5000);

    // image
    event="linebot_image";
    value1 = "https://video.nextmag.com.tw/photo/2016/04/25/1461565395_f66f-tile_1461575684432_555961_ver1.0.jpg";
    value2 = "https://video.nextmag.com.tw/photo/2016/01/26/B766ENT02-01_1453804306593_487920_ver1.0.jpg";
    request = "/trigger/linebot_image/with/key/"+key+"?value1="+value1+"&value2="+value2;
    Serial.println(tcp(domain,request,80,0)); 
    Serial.println();
    
    delay(5000);
    
    // vedio
    
    // location
  }
}

void loop() 
{
}

String tcp(String domain,String request,int port,byte wait)
{
    WiFiClient client_tcp;
    
    if (client_tcp.connect(domain, port)) 
    {
      Serial.println("GET " + request);
      client_tcp.println("GET " + request + " HTTP/1.1");
      client_tcp.println("Host: " + domain);
      //client_tcp.println("Connection: close");
      client_tcp.println();

      String getResponse="",Feedback="";
      boolean state = false;
      int waitTime = 3000;   // timeout 3 seconds
      long startTime = millis();
      while ((startTime + waitTime) > millis())
      {
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (state==true) Feedback += String(c);          
            if (c == '\n') 
            {
              if (getResponse.length()==0) state=true; 
              getResponse = "";
            } 
            else if (c != '\r')
              getResponse += String(c);
            if (wait==1)
              startTime = millis();
         }
         if (wait==0)
          if ((state==true)&&(Feedback.length()!= 0)) break;
      }
      client_tcp.stop();
      return Feedback;
    }
    else
      return "Connection failed";  
}
