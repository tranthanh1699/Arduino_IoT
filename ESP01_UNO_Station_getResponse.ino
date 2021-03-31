/* 
Arduino Uno + ESP-01 (AT Command)

Author : ChungYi Fu (Kaohsiung, Taiwan)  2018-08-07 22:00
https://www.facebook.com/francefu

Expanding Arduino Serial Port Buffer Size
https://internetofhomethings.com/homethings/?p=927
https://www.facebook.com/francefu/videos/10211231615856099/
*/

// Enter your WiFi ssid and password
String WIFI_SSID = "";   //your network SSID
String WIFI_PWD  = "";    //your network password

#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // ESP01 TX->D10, RX->D11 

String ReceiveData="",STAIP="",STAMAC="",ResponseData="";

void setup()
{
  Serial.begin(9600);
  
  //You must change uart baud rate of ESP-01 to 9600.
  mySerial.begin(115200);   //Default uart baud rate
  SendData("AT+UART_CUR=9600,8,1,0,0",2000);   //Change uart baud rate to 9600
  mySerial.begin(9600);  // you will get more stable data without junk chars.
  mySerial.setTimeout(10);

  SendData("AT+CIPSERVER=0",2000);
  SendData("AT+CWMODE_CUR=1",2000);
  SendData("AT+CIPMUX=0",2000);
  
  if (WIFI_SSID!="") 
    SendData("AT+CWJAP_CUR=\""+WIFI_SSID+"\",\""+WIFI_PWD+"\"",5000);  
  else
    Serial.print("Please check your network SSID and password settings");
}

void loop() 
{
  if (STAIP=="")
   {
    getSTAIP();
  
    if (STAIP!="")
    {
      pinMode(13,1);
      for (int i=0;i<20;i++)
      {
        digitalWrite(13,1);
        delay(50);
        digitalWrite(13,0);
        delay(50);
      }      
    }
  }
  else
  { 
    ResponseData="";
    String Domain="www.arduino.cc";
    String request = "GET / HTTP/1.1\r\nHost: "+Domain+"\r\n\r\n";
    /*
    If request length is too long, it can't work! (Arduino Uno)
    If you change buffer size to 256 bytes, request length must be less than or equal to 128.
    */    
    SendData("AT+CIPSTART=\"TCP\",\""+Domain+"\",80", 4000);
    SendData("AT+CIPSEND=" + String(request.length()+2), 2000);  //Serial.println(request) -> request+"\r\n" -> request.length()+2
    SendData(request, 6000);    // Adjust the waiting time until you get the complete response data.
    SendData("AT+CIPCLOSE",2000);
    
    Serial.println(ResponseData);  // If response length is too long, it will get junk chars because of serial buffer size.
    delay(10000);  
  }
}

void SendData(String data,int TimeLimit)
{
  mySerial.println(data);
  mySerial.flush();
  delay(10);
  WaitReply(TimeLimit);
}

String WaitReply(long int TimeLimit)
{
  ReceiveData="";
  long int StartTime=millis();
  while( (StartTime+TimeLimit) > millis())
  {
    if (mySerial.available())
    {
      delay(4);
      while(mySerial.available())
      {
        if (ReceiveData.indexOf("SEND OK")!=-1) 
          ResponseData=ResponseData+String(char(mySerial.read()));
        else
          ReceiveData=ReceiveData+String(char(mySerial.read()));
      }
      if ((ReceiveData.indexOf("OK")!=-1)&&(ReceiveData.indexOf("SEND OK")==-1)) 
        return ReceiveData;
    }
  } 
  return ReceiveData;
}

void getSTAIP()
{
  ReceiveData="";
  
  if (mySerial.available())
  {
    while (mySerial.available())
    {
      char c=mySerial.read();
      ReceiveData=ReceiveData+String(c);
    } 
    
    if (ReceiveData.indexOf("WIFI GOT IP")!=-1)
    { 
      while(!mySerial.find('OK')){} 
      delay(10);

      int staipreadstate=0,stamacreadstate=0,j=0;
      mySerial.println("AT+CIFSR");
      mySerial.flush();
      delay(6);
      
      while(mySerial.available())
      {
        char c=mySerial.read();
        String t=String(c);
        
        if (t=="\"") j++;
        
        if (j==1) 
          staipreadstate=1;
        else if (j==2)
          staipreadstate=0;
        if ((staipreadstate==1)&&(t!="\"")) STAIP=STAIP+t;

        if (j==3) 
          stamacreadstate=1;
        else if (j==4)
          stamacreadstate=0;
        if ((stamacreadstate==1)&&(t!="\"")) STAMAC=STAMAC+t;
        
        delay(1);
      } 
      Serial.println("STAIP: "+STAIP+"\nSTAMAC: "+STAMAC+"\n");
    }
  }
}
