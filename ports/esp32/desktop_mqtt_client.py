import paho.mqtt.client as mqtt
import cv2
import numpy as np
import queue
import time # Ensure time is imported at the top for clarity

# MQTT Broker details (should match the ESP32-CAM's configuration)
MQTT_BROKER = "nas.local" # e.g., "broker.hivemq.com"
MQTT_PORT = 1883
MQTT_TOPIC = "esp32/cam/image"

# Thread-safe queue for image data
image_queue = queue.Queue(maxsize=1) # Use a small queue to avoid memory issues if processing is slow

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe(MQTT_TOPIC)
    print(f"Subscribed to topic: {MQTT_TOPIC}")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(f"Message received on topic: {msg.topic}")
    if msg.topic == MQTT_TOPIC:
        try:
            image_data = msg.payload
            print(f"Received image data of size: {len(image_data)} bytes")
            
            # Put image data into the queue. Non-blocking if queue is full.
            try:
                image_queue.put_nowait(image_data)
                print("Image data put into queue.")
            except queue.Full:
                print("Image queue is full, dropping frame.")
        except Exception as e:
            print(f"Error processing message: {e}")
    else:
        print(f"Unknown topic: {msg.topic}")

def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        print(f"Attempting to connect to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}")
    except Exception as e:
        print(f"Could not connect to MQTT broker: {e}")
        return

    client.loop_start()
    print("MQTT client loop started.")

    try:
        while True:
            # Check for new image data in the queue
            try:
                image_data = image_queue.get_nowait()
                print("Image data retrieved from queue.")
                
                # Convert the byte array to a numpy array
                np_array = np.frombuffer(image_data, np.uint8)
                
                # Decode the image using OpenCV
                image = cv2.imdecode(np_array, cv2.IMREAD_COLOR)
                
                if image is not None:
                    print("Image decoded successfully.")
                    cv2.imshow("ESP32-CAM Image", image)
                    print("Image displayed.")
                else:
                    print("Failed to decode image.")
            except queue.Empty:
                pass # No new image, continue to keep window responsive

            key = cv2.waitKey(1) & 0xFF
            if key == ord('q'): # Press 'q' to quit
                break
            time.sleep(0.01) # Small delay to prevent busy-waiting
    except KeyboardInterrupt:
        pass
    finally:
        client.loop_stop()
        client.disconnect()
        cv2.destroyAllWindows()
        print("Application exited.")

if __name__ == "__main__":
    main()
