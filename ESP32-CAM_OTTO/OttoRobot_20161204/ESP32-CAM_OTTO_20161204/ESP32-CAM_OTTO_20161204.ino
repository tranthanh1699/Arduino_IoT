/*
ESP32-CAM OTTO ROBOT
Author : ChungYi Fu (Kaohsiung, Taiwan)  2020-2-17 19:00
https://www.facebook.com/francefu

ESP32-CAM RX -> Arduino NANO TX
ESP32-CAM TX -> Arduino NANO RX

http://APIP
http://STAIP

自訂指令格式 :  
http://APIP/?cmd=P1;P2;P3;P4;P5;P6;P7;P8;P9
http://STAIP/?cmd=P1;P2;P3;P4;P5;P6;P7;P8;P9

預設AP端IP： 192.168.4.1
http://192.168.xxx.xxx/?ip
http://192.168.xxx.xxx/?mac
http://192.168.xxx.xxx/?restart
http://192.168.xxx.xxx/?resetwifi=ssid;password
http://192.168.xxx.xxx/?inputpullup=pin
http://192.168.xxx.xxx/?pinmode=pin;value
http://192.168.xxx.xxx/?digitalwrite=pin;value
http://192.168.xxx.xxx/?analogwrite=pin;value
http://192.168.xxx.xxx/?digitalread=pin
http://192.168.xxx.xxx/?analogread=pin
http://192.168.xxx.xxx/?touchread=pin
http://192.168.xxx.xxx/?flash=value        //vale= 0~255  (閃光燈)
http://192.168.xxx.xxx/?otto=commandString

查詢Client端IP：
查詢IP：http://192.168.4.1/?ip
重設網路：http://192.168.4.1/?resetwifi=ssid;password

如果想快速執行指令不需等待回傳值，可在命令中增加參數值為stop。例如：
http://192.168.xxx.xxx/?digitalwrite=gpio;value;stop
http://192.168.xxx.xxx/?restart=stop
*/

//輸入WIFI連線帳號密碼
const char* ssid     = "*****";   //your network SSID
const char* password = "*****";   //your network password

//輸入AP端連線帳號密碼
const char* apssid = "ESP32-CAM";
const char* appassword = "12345678";         //AP端密碼至少要八個字元以上

#include <WiFi.h>
#include <WiFiClientSecure.h>    //用於https加密傳輸協定
#include <esp32-hal-ledc.h>      //用於控制伺服馬達
#include "esp_camera.h"          //視訊
#include "soc/soc.h"             //用於電源不穩不重開機       
#include "soc/rtc_cntl_reg.h"    //用於電源不穩不重開機

// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled

//安可信ESP32-CAM模組腳位設定
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer server(80);  //伺服器通信協定的埠號 80

String Feedback="";   //回傳客戶端訊息
String Command="",cmd="",P1="",P2="",P3="",P4="",P5="",P6="",P7="",P8="",P9="";   //指令參數值
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;  //指令拆解狀態值

void ExecuteCommand()
{
  //自訂指令區塊
  if (cmd=="your cmd") {
    // You can do anything
    // Feedback="<font color=\"red\">Hello World</font>";   //可為一般文字或HTML語法
  }
  else if (cmd=="ip") {  //查詢IP
    Feedback="AP IP: "+WiFi.softAPIP().toString();
    Feedback+=", ";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  }  
  else if (cmd=="mac") {  //查詢MAC
    Feedback="STA MAC: "+WiFi.macAddress();
  }  
  else if (cmd=="restart") {
    ESP.restart();
  }    
  else if (cmd=="resetwifi") {  //重設WIFI連線
    WiFi.begin(P1.c_str(), P2.c_str());
    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    } 
    Feedback="STAIP: "+WiFi.localIP().toString();
  }    
  else if (cmd=="inputpullup") {
    pinMode(P1.toInt(), INPUT_PULLUP);
  }  
  else if (cmd=="pinmode") {
    if (P2.toInt()==1)
      pinMode(P1.toInt(), OUTPUT);
    else
      pinMode(P1.toInt(), INPUT);
  }        
  else if (cmd=="digitalwrite") {
    ledcDetachPin(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  }   
  else if (cmd=="digitalread") {
    Feedback=String(digitalRead(P1.toInt()));
  }
  else if (cmd=="analogwrite") {
    if (P1="4") {
      ledcAttachPin(4, 4);  
      ledcSetup(4, 5000, 8);   
      ledcWrite(4,P2.toInt());  
    }
    else {
      ledcAttachPin(P1.toInt(), 5);
      ledcSetup(5, 5000, 8);
      ledcWrite(5,P2.toInt());
    }
  }       
  else if (cmd=="analogread") {
    Feedback=String(analogRead(P1.toInt()));
  }
  else if (cmd=="touchread") {
    Feedback=String(touchRead(P1.toInt()));
  }       
  else if (cmd=="framesize") { 
    sensor_t * s = esp_camera_sensor_get();  
    if (P1=="QQVGA")
      s->set_framesize(s, FRAMESIZE_QQVGA);
    else if (P1=="HQVGA")
      s->set_framesize(s, FRAMESIZE_HQVGA);
    else if (P1=="QVGA")
      s->set_framesize(s, FRAMESIZE_QVGA);
    else if (P1=="CIF")
      s->set_framesize(s, FRAMESIZE_CIF);
    else if (P1=="VGA")
      s->set_framesize(s, FRAMESIZE_VGA);  
    else if (P1=="SVGA")
      s->set_framesize(s, FRAMESIZE_SVGA);
    else if (P1=="XGA")
      s->set_framesize(s, FRAMESIZE_XGA);
    else if (P1=="SXGA")
      s->set_framesize(s, FRAMESIZE_SXGA);
    else if (P1=="UXGA")
      s->set_framesize(s, FRAMESIZE_UXGA);           
    else 
      s->set_framesize(s, FRAMESIZE_QVGA);     
  }   
  else if (cmd=="flash") {  //控制內建閃光燈
    ledcAttachPin(4, 4);  
    ledcSetup(4, 5000, 8);   
     
    int val = P1.toInt();
    ledcWrite(4,val);  
  }   
  else if (cmd=="otto") {
    P1 = urldecode(P1);   
    if (P1!="") Serial.println(P1); 
  }   
  else {
    Feedback="Command is not defined";
  }
  if (Feedback=="") Feedback=Command;  //若沒有設定回傳資料就回傳Command值
}


//自訂網頁首頁管理介面
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
  <!DOCTYPE html>
  <head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  </head><body>
  <script>var myVar;</script>
  <img id="stream" src=""> 
  <table>
    <tr>
      <td><input type="button" id="restart" value="Restart"></td>
      <td><input type="button" id="getStream" value="Get Stream"></td>   
      <td><input type="button" id="stopStream" value="Stop Stream"></td>
    </tr> 
    <tr>
      <td>Flash</td>
      <td colspan="2"><input type="range" id="flash" min="0" max="255" value="0"></td>
    </tr>
    <tr>
      <td>Resolution</td> 
      <td>
      <select id="framesize">
          <option value="UXGA">UXGA(1600x1200)</option>
          <option value="SXGA">SXGA(1280x1024)</option>
          <option value="XGA">XGA(1024x768)</option>
          <option value="SVGA">SVGA(800x600)</option>
          <option value="VGA">VGA(640x480)</option>
          <option value="CIF">CIF(400x296)</option>
          <option value="QVGA" selected="selected">QVGA(320x240)</option>
          <option value="HQVGA">HQVGA(240x176)</option>
          <option value="QQVGA">QQVGA(160x120)</option>
      </select> 
      </td>
      <td>
      </td>
    </tr>
    <tr>
      <td align="center"></td>
      <td align="center"><button onclick="try{fetch(location.origin+'/?otto=U');}catch(e){}">Front</button></td>
      <td align="center"></td>
    </tr>
    <tr>
      <td align="center"><button onclick="try{fetch(location.origin+'/?otto=L');}catch(e){}">Left</button></td>
      <td align="center"><button onclick="try{fetch(location.origin+'/?otto=B');}catch(e){}">Home</button></td>
      <td align="center"><button onclick="try{fetch(location.origin+'/?otto=R');}catch(e){}">Right</button></td>
    </tr>
    <tr>
      <td align="center"></td>
      <td align="center"><button onclick="try{fetch(location.origin+'/?otto=D');}catch(e){}">Back</button></td>
      <td align="center"></td>
    </tr>                   
    <tr>
      <td colspan="3">
      </td>
    </tr>
    <tr>
      <td colspan="3">
      <button onclick="try{fetch(location.origin+'/?otto=B');}catch(e){}">B</button>
      <button onclick="try{fetch(location.origin+'/?otto=A');}catch(e){}">A</button>
      <button onclick="try{fetch(location.origin+'/?otto=U');}catch(e){}">U</button>
      <button onclick="try{fetch(location.origin+'/?otto=D');}catch(e){}">D</button>
      <button onclick="try{fetch(location.origin+'/?otto=C');}catch(e){}">C</button>
      <button onclick="try{fetch(location.origin+'/?otto=L');}catch(e){}">L</button>
      <button onclick="try{fetch(location.origin+'/?otto=R');}catch(e){}">R</button>
      <button onclick="try{fetch(location.origin+'/?otto=a');}catch(e){}">a</button>
      <button onclick="try{fetch(location.origin+'/?otto=b');}catch(e){}">b</button>
      <button onclick="try{fetch(location.origin+'/?otto=c');}catch(e){}">c</button>
      </td>
    </tr>
    <tr>
      <td colspan="3">
      <button onclick="try{fetch(location.origin+'/?otto=d');}catch(e){}">d</button>
      <button onclick="try{fetch(location.origin+'/?otto=e');}catch(e){}">e</button>      
      <button onclick="try{fetch(location.origin+'/?otto=f');}catch(e){}">f</button>
      <button onclick="try{fetch(location.origin+'/?otto=g');}catch(e){}">g</button>
      <button onclick="try{fetch(location.origin+'/?otto=h');}catch(e){}">h</button>
      <button onclick="try{fetch(location.origin+'/?otto=i');}catch(e){}">i</button>
      <button onclick="try{fetch(location.origin+'/?otto=j');}catch(e){}">j</button>
      <button onclick="try{fetch(location.origin+'/?otto=k');}catch(e){}">k</button>
      <button onclick="try{fetch(location.origin+'/?otto=l');}catch(e){}">l</button>
      <button onclick="try{fetch(location.origin+'/?otto=m');}catch(e){}">m</button>      
      </td>
    </tr>
    <tr>
      <td colspan="3">
      <button onclick="try{fetch(location.origin+'/?otto=n');}catch(e){}">n</button>
      <button onclick="try{fetch(location.origin+'/?otto=o');}catch(e){}">o</button>
      <button onclick="try{fetch(location.origin+'/?otto=p');}catch(e){}">p</button>
      <button onclick="try{fetch(location.origin+'/?otto=q');}catch(e){}">q</button>      
      <button onclick="try{fetch(location.origin+'/?otto=r');}catch(e){}">r</button>
      <button onclick="try{fetch(location.origin+'/?otto=s');}catch(e){}">s</button>
      <button onclick="try{fetch(location.origin+'/?otto=t');}catch(e){}">t</button>
      <button onclick="try{fetch(location.origin+'/?otto=u');}catch(e){}">u</button>
      <button onclick="try{fetch(location.origin+'/?otto=v');}catch(e){}">v</button>
      <button onclick="try{fetch(location.origin+'/?otto=w');}catch(e){}">w</button>
      </td>
    </tr>
    <tr>
      <td colspan="3">
      <button onclick="try{fetch(location.origin+'/?otto=x');}catch(e){}">x</button>
      </td>
    </tr>    
  </table>  
  <iframe id="ifr" style="display:none"></iframe>
  <div id="message" style="color:red"><div>
  </div>
  </body>
  </html> 
  
  <script>
    var getStream = document.getElementById('getStream');
    var stopStream = document.getElementById('stopStream');
    var stream = document.getElementById('stream');
    var framesize = document.getElementById('framesize');
    var flash = document.getElementById('flash');
    var message = document.getElementById('message');
    var myTimer;
    var streamState = false;
    var restartCount=0;
  
    getStream.onclick = function (event) {
      clearInterval(myTimer);
      if (streamState == false) {
        myTimer = setInterval(function(){error_handle();},5000);         
        stream.src=location.origin+'/?getstill='+Math.random();
      }
      else
        streamState = false;
    }

    function error_handle() {
      restartCount++;
      clearInterval(myTimer);
      if (restartCount<=2) {
        message.innerHTML = "Get still error. <br>Restart ESP32-CAM "+restartCount+" times.";
        myTimer = setInterval(function(){getStream.click();},10000);
        //ifr.src = document.location.origin+'?restart';
      }
      else
        message.innerHTML = "Get still error. Please check ESP32-CAM.";
    }      
    
    stopStream.onclick = function (event) {
      clearInterval(myTimer);
      message.innerHTML = "";
      streamState=true;
      stream.src="";      
    }

    stream.onload = function (event) {
      clearInterval(myTimer);
      restartCount=0;      
      try { 
        document.createEvent("TouchEvent");
        setTimeout(function(){getStream.click();},250);
      }
      catch(e) { 
        setTimeout(function(){getStream.click();},150);
      }    
    }       
    
    restart.onclick = function (event) {
      try{      
        fetch(location.origin+'/?restart');
      }
      catch(e){}      
    }    
     
    framesize.onclick = function (event) {
      try{      
        fetch(location.origin+'/?framesize='+this.value);
      }
      catch(e){}      
    }  
    
    flash.onchange = function (event) {
      try{      
        fetch(location.origin+'/?flash='+this.value);
      }
      catch(e){}      
    }     
    
    </script>  
)rawliteral";

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //關閉電源不穩就重開機的設定
  
  Serial.begin(9600);
  //Serial.setDebugOutput(true);  //開啟診斷輸出

  //視訊組態設定
  camera_config_t config;  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  //https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h
  config.pixel_format = PIXFORMAT_JPEG;    //影像格式：RGB565|YUV422|GRAYSCALE|JPEG|RGB888|RAW|RGB444|RGB555
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  //視訊初始化
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    delay(1000);
    ESP.restart();
  }

  //可動態改變視訊框架大小(解析度大小)
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA

  //閃光燈(GPIO4)
  ledcAttachPin(4, 4);  
  ledcSetup(4, 5000, 8);   
  
  WiFi.mode(WIFI_AP_STA);  //其他模式 WiFi.mode(WIFI_AP); WiFi.mode(WIFI_STA);

  //指定Client端靜態IP
  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));

  WiFi.begin(ssid, password);    //執行網路連線

  delay(1000);
  
  long int StartTime=millis();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime+10000) < millis()) break;    //等待10秒連線
  } 

  if (WiFi.status() == WL_CONNECTED) {    //若連線成功
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);   //設定SSID顯示客戶端IP

    for (int i=0;i<5;i++) {   //若連上WIFI設定閃光燈快速閃爍
      ledcWrite(4,10);
      delay(200);
      ledcWrite(4,0);
      delay(200);    
    }    
  }
  else {
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);    

    for (int i=0;i<2;i++) {    //若連不上WIFI設定閃光燈慢速閃爍
      ledcWrite(4,10);
      delay(1000);
      ledcWrite(4,0);
      delay(1000);    
    }      
  }     

  //指定AP端IP
  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  
  server.begin();  

  //設定閃光燈為低電位
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);      
}

void loop() {
  Feedback="";Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
   WiFiClient client = server.available();

  if (client) { 
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        
        getCommand(c);   //將緩衝區取得的字元猜解出指令參數
                
        if (c == '\n') {
          if (currentLine.length() == 0) {    
            /*  
            //回傳JSON格式
            client.println("HTTP/1.1 200 OK");
            client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
            client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
            client.println("Content-Type: application/json;charset=utf-8");
            client.println("Access-Control-Allow-Origin: *");
            //client.println("Connection: close");
            client.println();
            client.println("[{\"esp32\":\""+Feedback+"\"}]");
            client.println();
            */
            
            /*
            //回傳XML格式
            client.println("HTTP/1.1 200 OK");
            client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
            client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
            client.println("Content-Type: text/xml; charset=utf-8");
            client.println("Access-Control-Allow-Origin: *");
            //client.println("Connection: close");
            client.println();
            client.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
            client.println("<esp32><feedback>"+Feedback+"</feedback></esp32>");
            client.println();
            */

            /*
            //回傳TEXT格式
            client.println("HTTP/1.1 200 OK");
            client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
            client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
            client.println("Content-Type: text/plain; charset=utf-8");
            client.println("Access-Control-Allow-Origin: *");
            client.println("Connection: close");
            client.println();
            client.println(Feedback);
            client.println();
            */

            if (cmd=="getstill") {
              //回傳JPEG格式影像
              camera_fb_t * fb = NULL;
              fb = esp_camera_fb_get();  
              if(!fb) {
                Serial.println("Camera capture failed");
                delay(1000);
                ESP.restart();
              }
  
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");              
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: image/jpeg");
              client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\""); 
              client.println("Content-Length: " + String(fb->len));             
              client.println("Connection: close");
              client.println();
              
              uint8_t *fbBuf = fb->buf;
              size_t fbLen = fb->len;
              for (size_t n=0;n<fbLen;n=n+1024) {
                if (n+1024<fbLen) {
                  client.write(fbBuf, 1024);
                  fbBuf += 1024;
                }
                else if (fbLen%1024>0) {
                  size_t remainder = fbLen%1024;
                  client.write(fbBuf, remainder);
                }
              }  
              
              esp_camera_fb_return(fb);
            
              pinMode(4, OUTPUT);
              digitalWrite(4, LOW);               
            }
            else {
              //回傳HTML格式
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: text/html; charset=utf-8");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Connection: close");
              client.println();
              
              String Data="";
              if (cmd!="")
                Data = Feedback;
              else {
                Data = String((const char *)INDEX_HTML);
              }
              int Index;
              for (Index = 0; Index < Data.length(); Index = Index+1000) {
                client.print(Data.substring(Index, Index+1000));
              }           
              
              client.println();
            }
                        
            Feedback="";
            break;
          } else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1)) {
          if (Command.indexOf("stop")!=-1) {  //若指令中含關鍵字stop立即斷線 -> http://192.168.xxx.xxx/?cmd=aaa;bbb;ccc;stop
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

//拆解命令字串置入變數
void getCommand(char c) {
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) P1=P1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) P2=P2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) P3=P3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) P4=P4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) P5=P5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) P6=P6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) P7=P7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) P8=P8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) P9=P9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}

//https://github.com/zenmanenergy/ESP8266-Arduino-Examples/blob/master/helloWorld_urlencoded/urlencode.ino
String urldecode(String str)
{
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

unsigned char h2int(char c)
{
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
