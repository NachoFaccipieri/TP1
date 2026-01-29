from flask import Flask, jsonify
from flask_cors import CORS
import time
import random

app = Flask(__name__)
CORS(app)

# Datos de ejemplo para simular la respuesta de la API
# En un caso real, estos datos vendrían de una base de datos o de sensores
def generate_data(num_points=50):
    data = []
    for i in range(num_points):
        timestamp = int(time.time()) - (num_points - 1 - i) * 60
        temperature = 20 + random.uniform(-2, 2)
        humidity_air = 60 + random.uniform(-5, 5)
        humidity_soil = 45 + random.uniform(-10, 10)
        data.append({
            'timestamp': timestamp,
            'temperature': temperature,
            'humidity_air': humidity_air,
            'humidity_soil': humidity_soil
        })
    return data

@app.route('/api/data', methods=['GET'])
def get_data():
    data = generate_data()
    return jsonify(data)

if __name__ == '__main__':
    app.run(debug=True)
