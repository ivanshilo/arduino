import paho.mqtt.publish as publish
import paho.mqtt.subscribe as subscribe
import paho.mqtt.client as mqtt
import time
import base64

meta = True
file_close = False

filename = ""

local_feed_name = 'camPIC'
local_feed_name_command = 'snapPIC'
local_feed_name_log = 'camLOG'
local_mqtt_server_name = 'asus-lap'

username = 'sundukevi4'
password_remote_mqtt = '933f50bb32a545e9bfa07d70a049902d'
remote_mqtt_server_name = 'io.adafruit.com'
remote_feed_name = username + '/feeds/camPIC'
remote_feed_name_command = username + '/feeds/snapPIC'
remote_feed_name_log = username + '/feeds/camLOG'

# The callback for when the client receives a CONNACK response from the server.
def on_connect_local(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe(local_feed_name)
    client.subscribe(local_feed_name_log)

# The callback for when a PUBLISH message is received from the server.
def on_message_local(client, userdata, msg):
    print(msg.topic + " " + str(msg.payload))
    if len(msg.payload) == 0:
	print("no data in payload, exit...")
	return
    
    if msg.topic == local_feed_name:
        record_image_from_camera(msg)
    if msg.topic == local_feed_name_log:
        client_remote.publish(remote_feed_name_log, msg.payload)
    
def record_image_from_camera(msg):
    global meta
    global file_close
    global f
    global filename
            
    # we expect for metadata to be sent now
    if meta:
	if msg.payload[0] == "S":
          # we have to create a new file
          if not f:
                timestr = time.strftime("%Y%m%d-%H%M%S")
                filename = 'picture_' + timestr + '.jpg'
                f = open(filename, 'wb')
                print("new file " + filename + " created")
                file_close = False
                
          print("S")
	  meta = False          
          return
      
        # next data packet is the last, be ready to close the file
	if msg.payload[0] == "E":
          print("E")
          meta = False
          file_close = True
	return

    # real data	
    if meta is not True:
        print("data")
        f.write(msg.payload)
	meta = True
        if file_close:
           f.close()
	   f = 0
           print("file closed")
           publish_to_remote_mqtt(client_remote, filename, remote_feed_name)
           
           
# The callback for when the client receives a CONNACK response from the server.
def on_connect_remote(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    # DEBUG only - to show what has been published by script
    #client.subscribe(remote_feed_name)
    client.subscribe(remote_feed_name_command, qos = 1)

# The callback for when a PUBLISH message is received from the server.
def on_message_remote(client, userdata, msg):
    print("Received message '" + str(msg.payload) + "' on topic '"
        + msg.topic + "' with QoS " + str(msg.qos))
    if msg.topic == remote_feed_name_command:
        client_local.publish(local_feed_name_command, msg.payload)
    
# publish data to remote mqtt server from file
def publish_to_remote_mqtt(client, filename, feedname):
    print("publishing to " + remote_mqtt_server_name + " file " + filename)
    image = open(filename)
    encoded = base64.b64encode(image.read())
    #for DEBUG
    #print encoded
    #publish to feed in internet
    client.publish(feedname, encoded)
          

client_remote = mqtt.Client()
client_remote.on_connect = on_connect_remote
client_remote.on_message = on_message_remote
client_remote.username_pw_set(username, password_remote_mqtt)
client_remote.connect(remote_mqtt_server_name, 1883, 60)


client_local = mqtt.Client()
client_local.on_connect = on_connect_local
client_local.on_message = on_message_local

client_local.connect(local_mqtt_server_name, 1883, 60)
f = 0
# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.

#loop_start() starts processing loop inbackground and returns to current thread
client_remote.loop_start()
# this loop blocks program from exiting
client_local.loop_forever()
