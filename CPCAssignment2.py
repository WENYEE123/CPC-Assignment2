import pymongo
import paho.mqtt.client as mqtt
from datetime import datetime, timezone
from google.cloud import bigquery
from google.oauth2 import service_account

# MongoDB configuration
mongo_client = pymongo.MongoClient("mongodb://localhost:27017/")
db = mongo_client["Flood"]
collection = db["iot"]

# BigQuery configuration
# Path to your service account key file
key_path = "upheld-world-446702-g9-42da1d30cfdc.json"

# Define required BigQuery scopes
scopes = ["https://www.googleapis.com/auth/bigquery"]

# Load the credentials with the required scopes
credentials = service_account.Credentials.from_service_account_file(key_path, scopes=scopes)

# Create the BigQuery client
bigquery_client = bigquery.Client(credentials=credentials, project=credentials.project_id)
dataset_id = "CPCAssignment2"  # Replace with your BigQuery dataset ID
table_id = f"CPCAssignment2.IoT"  # Replace with your BigQuery table name

# MQTT configuration
mqtt_broker_address = "35.193.17.193"
mqtt_topic = "iot"

# Define the callback function for connection
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Successfully connected")
        client.subscribe(mqtt_topic)

# Function to insert data into BigQuery
def insert_into_bigquery(data):
    rows_to_insert = [
        {
            "timestamp": data["timestamp"],
            "temperature": data["temperature"],
            "humidity": data["humidity"],
            "water_level": data["water_level"],
            "rain_status": data["rain_status"],
        }
    ]
    errors = bigquery_client.insert_rows_json(table_id, rows_to_insert)
    if errors == []:
        print("New row added to BigQuery.")
    else:
        print("Failed to insert rows:", errors)

# Parse the payload into structured data
def parse_payload(payload):
    try:
        # Extract fields from the payload string
        fields = payload.split(", ")
        temperature = float(fields[0].split(": ")[1].replace("°C", ""))
        humidity = float(fields[1].split(": ")[1].replace("%", ""))
        water_level = float(fields[2].split(": ")[1])  # Changed to float
        rain_status = fields[3].split(": ")[1]

        return {
            "temperature": temperature,
            "humidity": humidity,
            "water_level": water_level,
            "rain_status": rain_status,
        }
    except Exception as e:
        print(f"Failed to parse payload: {e}")
        return None

# Define the callback function for ingesting data into MongoDB and BigQuery
def on_message(client, userdata, message):
    payload = message.payload.decode("utf-8")
    print(f"Received message: {payload}")

    # Convert MQTT timestamp to datetime
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.%fZ")

    # Parse the payload
    structured_data = parse_payload(payload)
    if structured_data:
        # Insert data into MongoDB
        document = {"timestamp": timestamp, **structured_data}
        collection.insert_one(document)
        print("Data ingested into MongoDB")

        # Insert data into BigQuery
        structured_data["timestamp"] = timestamp  # Add the timestamp to BigQuery data
        insert_into_bigquery(structured_data)

# Create an MQTT client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Attach the callbacks using explicit methods
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Start the MQTT loop
client.loop_forever()
