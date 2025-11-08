from flask import Flask, jsonify
from pymongo import MongoClient
from flask_cors import CORS  # <--- importar

app = Flask(__name__)
CORS(app)  # <--- permitir CORS para todos los dominios

client = MongoClient("mongodb://localhost:27018/")  # tu puerto
db = client["datosPrueba"]
collection = db["datos"]

@app.route("/api/data")
def get_data():
    data = list(collection.find().sort("timestamp", -1).limit(10))
    for d in data:
        d["_id"] = str(d["_id"])
    return jsonify(data)

if __name__ == "__main__":
    app.run(debug=True)

