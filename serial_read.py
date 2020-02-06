#!/usr/bin/env python
import time
import serial
import requests
from datetime import datetime
import paho.mqtt.client as mqtt
import socket

from array import *

serial = serial.Serial(
port='/dev/ttyUSB_rpi',
baudrate = 115200
)

ourClient = mqtt.Client("rpi")


print("HAB SensPak receiver")
print("MQTT broker --> 192.168.0.100. Topic --> senspakData")

def is_connected():
    try:
        socket.create_connection(("www.google.com",80))
        return 1
    except:
        return 0
def send_data_http(data_to_send):
    try:
        r = requests.post('https://reqres.in/api/users',json=data_to_send)
        print("HTTP Server: SUCCESS")
    except:
        print("HTTP Server: FAILED")
        
def send_data_mqtt_remote(data_to_send):
    try:
        ourClient.connect("test.mosquitto.org",1883)
        ourClient.publish("senspakData",str(data_to_send))
        print("MQTT Server (remote): SUCCESS")
    except:
        print("MQTT Server (remote): FAILED")
    
def send_data_mqtt_local(data_to_send):
    try:
        ourClient.connect("192.168.0.100",1883)
        ourClient.publish("senspakData",str(data_to_send))
        print("MQTT Server (local): SUCCESS")
    except:
        print("MQTT Server (local): FAILED")
    
    
    
while (1):
   now = datetime.now()
   dateTime = now.strftime("%d/%m/%y %H:%M:%S")
   s = serial.readline()
   s = s.replace('\r','')
   s = s.replace('\n','')
   if s == "":
       print("empty")
   else: 
       data = s.split(",")
       try:
           sp_id = data[0]
           ec = data[1]
           do = data[2]
           temp = data[3]
           pH = data[4]
           wp = data[5]
           chl = data[6]
           batt = data[7]
           payload = {'id':sp_id,'ec':ec,'do':do,'temp':temp,'pH':pH,'wp':wp,'chl':chl,'batt':batt,'datetime':dateTime}
       except:
           payload ={'id':0,'ec':0,'do':0,'temp':0,'pH':0,'wp':0,'chl':0,'batt':0,'datetime':dateTime}

       print("Receiving transmission from SensPak no. {}...".format(sp_id))
       print("{}:{}".format(s,str(dateTime)))
       
       if is_connected() == 1:
           print("Online")
           send_data_http(payload)
           send_data_mqtt_remote(payload)
           send_data_mqtt_local(payload)
       else:
           print("Offline")
           send_data_mqtt_local(payload)
    
       
       f = open("HAB_data.txt", "a")
       f.write(str(payload))
       f.write(",")
       f.write("\n");
       f.close()
       
       time.sleep(7)
       serial.flushInput()
       
       




