from flask import Flask, jsonify, request
from pymongo import MongoClient
from flask_cors import CORS
from datetime import datetime
import base64

app = Flask(__name__)
CORS(app)

client = MongoClient("mongodb://localhost:27018/")
db = client["datosPrueba"]
collection = db["datos"]

# Endpoint para LEER datos (dashboard)
@app.route("/api/data")
def get_data():
    data = list(collection.find().sort("timestamp", -1).limit(10))
    for d in data:
        d["_id"] = str(d["_id"])
    return jsonify(data)

# Endpoint para RECIBIR datos del RAK
@app.route("/api/lora", methods=["POST"])
def receive_lora():
    try:
        data = request.json
        print(f"Recibido del RAK: {data}")
        
        # Decodificar Base64 del payload
        if "data" in data and data.get("data_encode") == "base64":
            payload_b64 = data["data"]
            payload_bytes = base64.b64decode(payload_b64)
            payload_str = payload_bytes.decode('utf-8')
            
            print(f"Payload decodificado: {payload_str}")
            
            # Parsear los valores (formato: temp:24.5,hum_air:58,luz:44,hum_soil:42,count:360)
            sensor_data = {}
            for pair in payload_str.split(','):
                if ':' in pair:
                    key, value = pair.split(':')
                    sensor_data[key] = value
            
            # Crear documento para MongoDB con datos parseados
            document = {
                "timestamp": datetime.now().isoformat(),
                "deviceName": data.get("deviceName"),
                "temp": float(sensor_data.get("temp", 0)),
                "hum_air": int(sensor_data.get("hum_air", 0)),
                "luz": int(sensor_data.get("luz", 0)),
                "hum_soil": int(sensor_data.get("hum_soil", 0)),
                "count": int(sensor_data.get("count", 0)),
                "rssi": data.get("rxInfo", [{}])[0].get("rssi", 0) if data.get("rxInfo") else 0,
                "snr": data.get("rxInfo", [{}])[0].get("loRaSNR", 0) if data.get("rxInfo") else 0,
                "raw_payload": payload_str
            }
        else:
            # Si no tiene data en base64, guardar tal cual
            document = data
            document["timestamp"] = datetime.now().isoformat()
        
        # Guardar en MongoDB
        collection.insert_one(document)
        
        return jsonify({"status": "ok"}), 200
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"error": str(e)}), 500

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000, debug=True)

