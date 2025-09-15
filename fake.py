from pymongo import MongoClient
import random, time

client = MongoClient("mongodb://localhost:27018/")
db = client["datosPrueba"]
collection = db["datos"]

for _ in range(10):
    doc = {
        "timestamp": int(time.time()),
        "temperature": round(20 + random.random() * 5, 2),
        "humidity_air": round(40 + random.random() * 20, 2),
        "humidity_soil": round(30 + random.random() * 15, 2)
    }
    collection.insert_one(doc)
    time.sleep(1)

