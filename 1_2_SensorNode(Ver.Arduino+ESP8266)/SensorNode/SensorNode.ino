#include <SoftwareSerial.h>
#include <DHT11.h>
#include "A_ESP8266.h"

#define PWM_LED_PIN 9
#define DHT_PIN 12
#define ESP_TX  10
#define ESP_RX  11

#define SSID        "DKWS"
#define PASSWORD    "110258502241"
#define HOST_ADDR   "192.168.1.10"
#define HOST_PORT   (25602)

int err;
DHT11 dht11(DHT_PIN);
float   dht_temp, dht_humi;

SoftwareSerial esp8266(ESP_RX, ESP_TX); //RX 11, TX 10
A_ESP8266 wifi(esp8266);

uint32_t TCP_Receive(unsigned char* buf)
{
  unsigned char recv_buf[256];
  memset(recv_buf, 0x00, sizeof(recv_buf));
  uint32_t len = wifi.recv(recv_buf, sizeof(recv_buf), 1000);

  if(recv_buf[1] == 0xFF)   return -1;

  if(len > 0)
  {
    memcpy(buf, recv_buf, len);
    return 1;
  }
  else    return 0;

}

void TCP_Send(unsigned char key, unsigned char msg_id, unsigned char body_size, unsigned char* body)
{
  int i;
  unsigned char send_buf[256];
  memset(send_buf, 0x00, sizeof(send_buf));

  *send_buf = key;
  *(send_buf+1) = msg_id;     //msg_id (temperature 0x01, humidity 0x02, light 0x04, switch 0x10)
  *(send_buf+2) = body_size;

  memcpy((send_buf+3), body, body_size);

  wifi.send(send_buf, body_size + 3);
}

void setup() {
  Serial.begin(9600);
  pinMode( PWM_LED_PIN, OUTPUT );
  analogWrite( PWM_LED_PIN, 0);
}

int ConnectStep = 1;
void loop() {
  #if 1
  switch(ConnectStep)
  {
  case 1:
    if(wifi.setOprToStationSoftAP())
    {
      Serial.print("[1] to station + softap ok\r\n");
      ConnectStep++;
    }
    break;

  case 2:
    if (wifi.joinAP(SSID, PASSWORD))
    {
      Serial.print("[2] Join AP success\r\n");
      Serial.println( wifi.getLocalIP().c_str());
      ConnectStep++;
    }
    break;

  case 3:
    if (wifi.disableMUX())
    {
      Serial.print("[3] single ok\r\n");
      ConnectStep++;
    }
    break;

  case 4:
    if (wifi.createTCP(HOST_ADDR, HOST_PORT))
    {
      Serial.print("[4] create tcp ok\r\n");
      if(DataNode_Main())
      {
        Serial.print("[!] Retry\r\n");
      }
    }
    break;
  }
  delay(500);
#else
  DataNode_Main();
#endif
}

void MakeFormat(unsigned char format, void* data, unsigned char* buf)
{
  float tmp_f;
  if(format & 0x01)
  {
    tmp_f = *(float*)data;
    buf[0] = (unsigned char)((int)tmp_f/10);
    buf[1] = (unsigned char)((int)tmp_f%10);
    tmp_f = (tmp_f - (int)tmp_f)*100;
    buf[2] = (unsigned char)((int)tmp_f/10);
    buf[3] = (unsigned char)((int)tmp_f%10);
  }
}

bool isLedOn = true;

void Sense_Proc(unsigned char mode, unsigned char* buf)
{
  int i;
  if(err = dht11.read(dht_humi, dht_temp) == 0)
  {
    MakeFormat(0x01, (void*)&dht_temp, &buf[3]);
    MakeFormat(0x01, (void*)&dht_humi, &buf[7]);
    
    //send packet
    TCP_Send(27, 0x03, 8, &buf[3]);
  }
  else
  {
    ;//send error packet
  }

  //if(mode)    delay(DHT11_RETRY_DELAY);
}

void Light_Proc(void)
{
  //toggle
  if(isLedOn)    isLedOn = false;
  else           isLedOn = true;

  //setting
  // - delay
  // - brightness
}

int DataNode_Main()
{
  int i;
  unsigned char buffer[256];
  
  while(1)
  {
    //========GET COMMAND========//
    if(TCP_Receive(buffer))
    {
      if(buffer[0] == 27)
      {
        if(buffer[1] & 0x80) {
          Serial.print("(!) Server closed\n");
          break;
        }

        if(buffer[1] & 0x01) {
          Sense_Proc(0, buffer);
          Serial.print("(!) Sense Proc\n");
        }
        
        if(buffer[1] & 0x02) {
          Light_Proc();
          Serial.print("(!) Light Proc\n");
        }
        
        //...
      }
    }
    
    //=======Sense Proc==========//
    Sense_Proc(1, buffer);
    
    //=======Light Proc==========//
    if(isLedOn)
    {
      for (i = 20; i < 256; i++)
      {
        analogWrite(PWM_LED_PIN, i);
        delay(10);
      }
      for (i = 255; i >= 20; i--)
      {
        analogWrite(PWM_LED_PIN, i);
        delay(10);
      }
    }
    else
    {
      analogWrite(PWM_LED_PIN, 0);
      delay(5000);
    }
  }
  return 1;
}

