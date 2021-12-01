#!/usr/bin/env python
import time
import serial
import requests
from datetime import datetime
import paho.mqtt.client as mqtt
import socket

from array import *

serial = serial.Serial(
port= '/dev/ttyS0',       #'/dev/ttyUSB0',
baudrate = 115200,
timeout=1
)

ourClient = mqtt.Client("rpi")


print("HAB SensPak receiver. MQTT broker --> 192.168.0.100. Topic --> senspakData")
print("\n")

def is_connected():
    try:
        socket.create_connection(("www.google.com",80))
        #print("Receiver is online")
        return 1
    except:
        return 0
##def send_data_http(data_to_send):
##    try:
##        r = requests.post('https://reqres.in/api/users',json=data_to_send)
##        print("HTTP Server: SUCCESS")
##    except:
##        print("HTTP Server: FAILED")
        
def send_data_mqtt_remote(data_to_send):
    try:
        ourClient.connect("202.90.159.84",1883)
        ourClient.publish("senspakdata",str(data_to_send))
        print("MQTT Server (remote): SUCCESS")
    except:
        print("MQTT Server (remote): FAILED")
    
##def send_data_mqtt_local(data_to_send):
##    try:
##        ourClient.connect("192.168.1.1",1883)
##        ourClient.publish("senspakData",str(data_to_send))
##        print("MQTT Server (local): SUCCESS")
##    except:
##        print("MQTT Server (local): FAILED")
    
    
    
while (1):
    try:
       s = serial.readline();
       #print(s)
       s = s.replace('\r','')
       s = s.replace('\n','')
      # s += ",0"
       if s == "":
           print("empty")
       else:
           #print(s)
           now = datetime.now()
           dateTime = now.strftime("%d/%m/%y %H:%M:%S")
           data = s.split(",")
           try:
               pass_code = data[0]
               sp_id = "sp" + data[1]
               ec = data[2]
               sal = data[3]
               do = data[4]
               temp = data[5]
               pH = data[6]
               chl = data[7]
               batt = data[8]
               payload = {'id':sp_id,'ec':ec,'sal':sal,'do':do,'temp':temp,'pH':pH,'chl':chl,'batt':batt,'datetime':dateTime}
           except:
               payload ={'id':0,'ec':0,'sal':0,'do':0,'temp':0,'pH':0,'chl':0,'batt':0,'datetime':dateTime}
           if (pass_code == "spdata"):
               print("Receiving transmission from SensPak no. {}...".format(sp_id))
               print("{},{}".format(s,str(dateTime)))
               #print(payload)
               f = open("HAB_data.txt", "a")
               f.write(str(payload))
               f.write(",")
               f.write("\n");
               f.close()
           
               if is_connected() == 1:
                   print("Online")
                   #send_data_http(payload)
                   send_data_mqtt_remote(payload)
                   #send_data_mqtt_local(payload)
               else:
                   print("Offline")
                   #send_data_mqtt_local(payload)
    ##       
    ##           f = open("HAB_data.txt", "a")
    ##           f.write(str(payload))
    ##           f.write(",")
    ##           f.write("\n");
    ##           f.close()
        
         
           time.sleep(1)
           serial.flushInput()
    except:
        print("waiting for serial")
               
        
           
           
           




