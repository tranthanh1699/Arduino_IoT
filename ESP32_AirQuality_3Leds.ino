/*
ESP32 AirQuality (Webbit)
Author : ChungYi Fu (Kaohsiung, Taiwan)  2021-1-20 00:30
https://www.facebook.com/francefu

MbitBot Lite 8  - >  Webbit P16
MbitBot Lite 9  - >  Webbit P17
MbitBot Lite 13 - >  Webbit P18

Set WIFI ssid and password
http://192.168.4.1?admin
http://STAIP?admin

Set Line Notify token
http://192.168.4.1?token
http://STAIP?token

Set Site
http://192.168.4.1?site
http://STAIP?site

Get sensor values
http://192.168.4.1?get
http://STAIP?get

Erase SPI Flash
http://192.168.4.1?eraseflash
http://STAIP?eraseflash
refer to
https://ruten-proteus.blogspot.com/2016/12/ESp8266ArduinoQA-02.html
http://wyj-learning.blogspot.com/2018/03/nodemcu-flash.html
  
ESP32 LCD Library
https://github.com/nhatuan84/esp32-lcd
16x2 LCD
5V, GND, SDA:gpio12 , SCL:gpio14 
[Web:Bit]
5V, GND, RX:P6 (gpio12), TX:P7 (gpio14)

Command Format :  
http://APIP/?cmd=p1;p2;p3;p4;p5;p6;p7;p8;p9
http://STAIP/?cmd=p1;p2;p3;p4;p5;p6;p7;p8;p9

Default APIP： 192.168.4.1

http://192.168.4.1/?ip
http://192.168.4.1/?mac
http://192.168.4.1/?restart
http://192.168.4.1/?resetwifi=ssid;password
http://192.168.4.1/?inputpullup=pin
http://192.168.4.1/?pinmode=pin;value
http://192.168.4.1/?digitalwrite=pin;value
http://192.168.4.1/?analogwrite=pin;value
http://192.168.4.1/?digitalread=pin
http://192.168.4.1/?analogread=pin
http://192.168.4.1/?touchread=pin
http://192.168.4.1/?linenotify=token;request
--> request = message=xxxxx
--> request = message=xxxxx&stickerPackageId=xxxxx&stickerId=xxxxx
*/

const char* ssid     = "";  //WIFI ssid
const char* password = "";  //WIFI pwd

const char* apssid = "ESP32_AIRQUALITY";
const char* appassword = "12345678";

// Site Name (Chinese)  https://opendata.epa.gov.tw/Data/Contents/AQI/
String Site = "小港";    //Opendata Site (Chinese)
String SiteName = "Xiaogang";    //Display in LCD

String line_token = "";  //Line Notify Token
int delaytime = 600;               //delay 600 seconds

#include <ArduinoJson.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
int lcdAddress = 39;    //0x27=39, 0x3F=63 
LiquidCrystal_I2C lcd(lcdAddress, 16, 2);
int lcdSDA = 12;
int lcdSCL = 14;

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <ESP.h>
const uint32_t addressStart = 0x3FA000; 
const int len = 64;    // flashWrite, flashRead -> i = 0 to 63

long pm25 = 0;
long AQI = 0;
String Status = "";
int delaycount = 0;

WiFiServer server(80);

String Feedback="",Command="",cmd="",p1="",p2="",p3="",p4="",p5="",p6="",p7="",p8="",p9="";
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;

void ExecuteCommand()
{
  Serial.println("");
  //Serial.println("Command: "+Command);
  Serial.println("cmd= "+cmd+" ,p1= "+p1+" ,p2= "+p2+" ,p3= "+p3+" ,p4= "+p4+" ,p5= "+p5+" ,p6= "+p6+" ,p7= "+p7+" ,p8= "+p8+" ,p9= "+p9);
  Serial.println("");

  if (cmd=="admin") { 
    Feedback="WIFI<br>SSID: <input type=\"text\" id=\"ssid\"><br>PWD: <input type=\"text\" id=\"pwd\"><br><input type=\"button\" value=\"submit\" onclick=\"location.href='?resetwifi='+document.getElementById('ssid').value+';'+document.getElementById('pwd').value;\">";  
  }
  else if (cmd=="token") { 
    Feedback="Line Notify Token: <input type=\"text\" id=\"token\"><input type=\"button\" value=\"submit\" onclick=\"location.href='?token='+document.getElementById('token').value;\">";  
  }  
  else if (cmd=="site") { 
    Feedback="Site: <input type=\"text\" id=\"site\">(Opendata site)<br>Site Name: <input type=\"text\" id=\"sitename\">(LCD, Line Notify)<br><input type=\"button\" value=\"submit\" onclick=\"location.href='?site='+document.getElementById('site').value+';'+document.getElementById('sitename').value;\">";  
  } 
  else if (cmd=="token") {
    char buff_token[len]; 
    strcpy(buff_token, p1.c_str());
    flashWrite(buff_token, 2); 
        
    line_token = p1;
    Feedback="Set Line Notify Token = "+p1+" OK";    
  } 
  else if (cmd=="site") {
    p1.replace("%","_");
    p2.replace("%","_");
    Site = p1;
    SiteName = p2;
    delaycount=delaytime;

    char buff_site[len], buff_sitename[len]; 
    strcpy(buff_site, p1.c_str());
    strcpy(buff_sitename, p2.c_str());
    flashWrite(buff_site, 3);
    flashWrite(buff_sitename, 4); 
    
    p1.replace("_","%");
    Feedback="Set Site = "+urldecode(p1)+", SiteName = "+p2+" OK";    
  }   
  else if (cmd=="get") {
    Feedback = "SITE: "+urldecode(Site)+"<br>SITENAME: "+SiteName+"<br>AQI:    " + String(AQI) + "   [<a href='https://airtw.epa.gov.tw/CHT/Information/Standard/AirQualityIndicator.aspx' target='_blank'>Indicator</a>]<br>PM2.5:    "+String(pm25)+" ug/m3";
  }
  else if (cmd=="eraseflash") {
    flashErase();
    Feedback="Erase SPI Flash OK. <a href=\"?restart\">Restart the board</a>";
  }   
  else if (cmd=="resetwifi") {
    char buff_ssid[len], buff_password[len]; 
    strcpy(buff_ssid, p1.c_str());
    strcpy(buff_password, p2.c_str());
    flashWrite(buff_ssid, 0);
    flashWrite(buff_password, 1);   

    WiFi.begin(p1.c_str(), p2.c_str());
    Serial.print("Connecting to ");
    Serial.println(p1);
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    } 
    Serial.println("");
    Serial.println("STAIP: "+WiFi.localIP().toString());
    Feedback="STAIP: "+WiFi.localIP().toString();
    if (WiFi.status() == WL_CONNECTED) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(WiFi.localIP().toString());
      delaycount=delaytime;     
    }
  }      
  else if (cmd=="ip") {
    Feedback="AP IP: "+WiFi.softAPIP().toString();    
    Feedback+=", ";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  }  
  else if (cmd=="mac") {
    Feedback="STA MAC: "+WiFi.macAddress();
  }  
  else if (cmd=="restart") {
    ESP.restart();
  } 
  else if (cmd=="inputpullup") {
    pinMode(p1.toInt(), INPUT_PULLUP);
  }  
  else if (cmd=="pinmode") {
    if (p2.toInt()==1)
      pinMode(p1.toInt(), OUTPUT);
    else
      pinMode(p1.toInt(), INPUT);
  }        
  else if (cmd=="digitalwrite") {
    ledcDetachPin(p1.toInt());
    pinMode(p1.toInt(), OUTPUT);
    digitalWrite(p1.toInt(), p2.toInt());
  }   
  else if (cmd=="digitalread") {
    Feedback=String(digitalRead(p1.toInt()));
  }
  else if (cmd=="analogwrite") {
    ledcAttachPin(p1.toInt(), 1);
    ledcSetup(1, 5000, 8);
    ledcWrite(1,p2.toInt());
  }       
  else if (cmd=="analogread") {
    Feedback=String(analogRead(p1.toInt()));
  }
  else if (cmd=="touchread") {
    Feedback=String(touchRead(p1.toInt()));
  }      
  else if (cmd=="linenotify") {    //message=xxx&stickerPackageId=xxx&stickerId=xxx
    String token = p1;
    String request = p2;
    Feedback=LineNotify(token,request,1);
  } 
  else {
    Feedback="Command is not defined.";
  }
  if (Feedback=="") Feedback=Command;  
}

void setup() {
  Serial.begin(115200);
  
  lcd.begin(lcdSDA, lcdSCL);
  lcd.backlight();  
  lcd.clear();   
    
  WiFi.mode(WIFI_AP_STA);

  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));

  WiFi.begin(ssid, password);

  delay(1000);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");       
    if ((StartTime+10000) < millis()) break;
  } 

  if (WiFi.status() == WL_CONNECTED) {    
    lcd.setCursor(0,0);
    lcd.print(WiFi.localIP().toString());
  }
  else {
    char buff_ssid[len], buff_password[len];
    strcpy(buff_ssid, flashRead(0));
    strcpy(buff_password, flashRead(1));
    if ((buff_ssid[0]>=32)&&(buff_ssid[0]<=126)) {
      Serial.println("");
      Serial.println("SPI Flash ssid = "+String(buff_ssid));
      Serial.println("SPI Flash password = "+String(buff_password));  
      
      WiFi.begin(buff_ssid, buff_password);
      Serial.print("\nConnecting to ");
      Serial.println(buff_ssid);
      
      long int StartTime=millis();
      while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");    
        if ((StartTime+5000) < millis()) break;
      }       
    }
    if (WiFi.status() == WL_CONNECTED) {  
      lcd.setCursor(0,0);
      lcd.print(WiFi.localIP().toString());
    }
  }

  Serial.println("");
  Serial.println("STAIP address: ");
  Serial.println(WiFi.localIP());
  
  if (WiFi.status() == WL_CONNECTED)
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);
  else
    WiFi.softAP(apssid, appassword);
    
  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  Serial.println("");
  Serial.println("APIP address: ");
  Serial.println(WiFi.softAPIP());   
   
  server.begin();  
  
  delay(3000); 

  if (Site!="")
    Site = urlencode(Site);

  for (int i=16;i<=18;i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
    delay(1000);
    digitalWrite(i, LOW); 
  }   
}

void loop() {

  getRequest();

  delay(1000);
  if (delaycount>0&&delaycount<delaytime) {
    delaycount++;
    return;
  }
  else
    delaycount=1;
  
  if (WiFi.status() == WL_CONNECTED) {
    AQI = 0;
    retrievepm25();

    if (AQI == 0)
      retrievepm25();

    Serial.println("");
    Serial.println("AQI : " + String(AQI));    
    Serial.println("pm2.5 : " + String(pm25) + " ug/m3");
    Serial.println("");
      
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(SiteName);
    lcd.setCursor(0,1);
    lcd.print("AQI=" + String(AQI) + "," + "PM25=" + String(pm25));

    //Line Notify
    if (line_token=="") {
      char buff_token[len];
      strcpy(buff_token, flashRead(2));
      if ((buff_token[0]>=32)&&(buff_token[0]<=126)) {
        line_token = String(buff_token);
        Serial.println("");
        Serial.println("SPI Flash token = "+line_token);
      }
    }
    if (line_token!="") {
      String message = "\nSITE: "+urldecode(Site) +"\nSITENAME: "+SiteName +"\nAQI: "+String(AQI)+"\nPM2.5: "+String(pm25)+"\nStatus: "+String(Status);
      Serial.println(LineNotify(line_token, "message="+message, 1));
    }
  }
}

void retrievepm25(){
 String Body = getAirQuality();
 
 Serial.println(Body);
 JsonObject obj;
 DynamicJsonDocument doc(1024); 
 deserializeJson(doc, Body);
 obj = doc.as<JsonObject>();
 
 AQI = obj["AQI"].as<String>().toInt();
 pm25 = obj["PM2.5"].as<String>().toInt();
 Status = obj["Status"].as<String>();

 if (AQI<=100) {
   digitalWrite(16, HIGH);
   digitalWrite(17, LOW);
   digitalWrite(18, LOW);
 }
 else if (AQI>100&&AQI<=200) {
   digitalWrite(16, LOW);
   digitalWrite(17, HIGH);
   digitalWrite(18, LOW);
 }
 else if (AQI>200) {
   digitalWrite(16, LOW);
   digitalWrite(17, LOW);
   digitalWrite(18, HIGH);
 } 
 
  /*
  obj["SiteName"].as<String>();             // "SiteName":"小港"
  obj["County"].as<String>();               // "County":"高雄市"
  obj["AQI"].as<String>().toInt();          // "AQI":"86"
  obj["Pollutant"].as<String>();            // "Pollutant":"細懸浮微粒"
  obj["Status"].as<String>();               // "Status":"普通"
  obj["SO2"].as<String>().toInt();          // "SO2":"4.2"
  obj["CO"].as<String>().toInt();           // "CO":"0.53"
  obj["CO_8hr"].as<String>().toInt();       // "CO_8hr":"0.5"
  obj["O3"].as<String>().toInt();           // "O3":"20.4"
  obj["O3_8hr"].as<String>().toInt();       // "O3_8hr":"43"
  obj["PM10"].as<String>().toInt();         // "PM10":"63"
  obj["PM2.5"].as<String>().toInt();        // "PM2.5":"30"
  obj["NO2"].as<String>().toInt();          // "NO2":"36.6"
  obj["NOx"].as<String>().toInt();          // "NOx":"37.6"
  obj["NO"].as<String>().toInt();           // "NO":"0.9"
  obj["WindSpeed"].as<String>().toInt();    // "WindSpeed":"1.5"
  obj["WindDirec"].as<String>().toInt();    // "WindDirec":"351"
  obj["PublishTime"].as<String>();          // "PublishTime":"2021/01/19 23:00:00"
  obj["PM2.5_AVG"].as<String>().toInt();    // "PM2.5_AVG":"30"
  obj["PM10_AVG"].as<String>().toInt();     // "PM10_AVG":"63"
  obj["SO2_AVG"].as<String>().toInt();      // "SO2_AVG":"3"
  obj["Longitude"].as<String>();            // "Longitude":"120.337736"
  obj["Latitude"].as<String>();             // "Latitude":"22.565833"
  obj["SiteId"].as<String>();               // "SiteId":"58"
  */ 
}

void getRequest() {
  Command="";cmd="";p1="";p2="";p3="";p4="";p5="";p6="";p7="";p8="";p9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
   WiFiClient client = server.available();

  if (client) 
  { 
    String currentLine = "";

    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();             
        
        getCommand(c);
                
        if (c == '\n') {
          if (currentLine.length() == 0) {       
            client.println("HTTP/1.1 200 OK");
            client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
            client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
            client.println("Content-Type: text/html; charset=utf-8");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html><head>");
            client.println("<meta charset=\"UTF-8\">");
            client.println("<meta http-equiv=\"Access-Control-Allow-Origin\" content=\"*\">");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
            client.println("</head><body>");
            client.println(Feedback);
            client.println("</body></html>");
            client.println();
                        
            Feedback="";
            break;
          } else {
            currentLine = "";
          }
        } 
        else if (c != '\r') 
        {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1))
        {
          if (Command.indexOf("stop")!=-1) {
            client.println();
            client.println();
            client.stop();
          }
          currentLine="";
          Feedback="";
          ExecuteCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void getCommand(char c)
{
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) p1=p1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) p2=p2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) p3=p3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) p4=p4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) p5=p5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) p6=p6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) p7=p7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) p8=p8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) p9=p9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}

String getAirQuality() {
    //Site
    if (Site=="") {
      char buff_site[len], buff_sitename[len];
      strcpy(buff_site, flashRead(3));
      strcpy(buff_sitename, flashRead(4));
      if ((buff_site[0]>=32)&&(buff_site[0]<=126)) {
        Site = String(buff_site);
        SiteName = String(buff_sitename); 
      }
      Serial.println("");
      Serial.println("SPI Flash Site = "+Site);
      Serial.println("SPI Flash SiteName = "+SiteName);
      Serial.println("");      
    }
      
    Site.replace("_","%");

    WiFiClientSecure client_tcp;

    String request = "/webapi/api/rest/datastore/355000000I-000259?sort=County&filters=SiteName%20eq%20%27"+Site+"%27";
    if (client_tcp.connect("opendata.epa.gov.tw", 443)) 
    {
      Serial.println("GET " + request);
      client_tcp.println("GET " + request + " HTTP/1.1");
      client_tcp.println("Host: opendata.epa.gov.tw");
      client_tcp.println("Connection: close");
      client_tcp.println();

      String getResponse="",Feedback="";
      boolean state = false;
      boolean cutstate = false;
      int waitTime = 20000;   // timeout 20 seconds
      long startTime = millis();
      while ((startTime + waitTime) > millis())
      {
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (state==true) {
              if (cutstate == false||(cutstate == true&&String(c)!="]")) {
                Feedback += String(c);
              }    
              if (cutstate == true&&String(c)=="]")
                state=false;          
              if (Feedback.indexOf("\"records\":[")!=-1) {
                Feedback="";
                cutstate = true;
              }
            }
            if (c == '\n') 
            {
              if (getResponse.length()==0) state=true; 
              getResponse = "";
            } 
            else if (c != '\r')
              getResponse += String(c);
            
            startTime = millis();
         }

         if (Feedback.length()!= 0) break;
      }
      client_tcp.stop();
      return Feedback;
    }
    else
      return "Connection failed";  
}

String LineNotify(String token, String request, byte wait)
{
  request.replace("%","%25");
  request.replace(" ","%20");
  request.replace("&","%20");
  request.replace("#","%20");
  //request.replace("\'","%27");
  request.replace("\"","%22");
  request.replace("\n","%0D%0A");
  request.replace("%3Cbr%3E","%0D%0A");
  request.replace("%3Cbr/%3E","%0D%0A");
  request.replace("%3Cbr%20/%3E","%0D%0A");
  request.replace("%3CBR%3E","%0D%0A");
  request.replace("%3CBR/%3E","%0D%0A");
  request.replace("%3CBR%20/%3E","%0D%0A"); 
  request.replace("%20stickerPackageId","&stickerPackageId");
  request.replace("%20stickerId","&stickerId");    

  WiFiClientSecure client_tcp;
  
  if (client_tcp.connect("notify-api.line.me", 443)) 
  {
    Serial.println(request);    
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close"); 
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("User-Agent: ESp8266/1.0");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Content-Length: " + String(request.length()));
    client_tcp.println();
    client_tcp.println(request);
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

void flashWrite(char data[len], int i) {      // len = 64 -> i = 0 to 63
  uint32_t flashAddress = addressStart + i*len;
  char buff_write[len];
  strcpy(buff_write, data);
  if (ESP.flashWrite(flashAddress,(uint32_t*)buff_write, sizeof(buff_write)-1))
    Serial.printf("address: %p write \"%s\" [ok]\n", flashAddress, buff_write);
  else 
    Serial.printf("address: %p write \"%s\" [error]\n", flashAddress, buff_write);
}

char* flashRead(int i) {      // len = 64 -> i = 0 to 63
  uint32_t flashAddress = addressStart + i*len;
  static char buff_read[len];
  if (ESP.flashRead(flashAddress,(uint32_t*)buff_read, sizeof(buff_read)-1)) {
    //Serial.printf("data: \"%s\"\n", buff_read);
    return buff_read;
  } else  
    return "";  
}

void flashErase() {
  if (ESP.flashEraseSector(addressStart / 4096))
    Serial.println("\nErase SPI Flash [ok]");
  else
    Serial.println("\nErase SPI Flash [error]");
}

/*
bool flashEraseSector(uint32_t sector);
bool flashWrite(uint32_t offset, uint32_t *data, size_t size);
bool flashRead(uint32_t offset, uint32_t *data, size_t size);
*/

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/
String urlencode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
}

String urldecode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

unsigned char h2int(char c) {
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
