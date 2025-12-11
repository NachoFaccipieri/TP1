import serial
import sys

# Cambia COM3 por el puerto de tu EDU-CIAA
puerto = 'COM3'  
baudios = 115200

try:
    ser = serial.Serial(puerto, baudios, timeout=1)
    print(f"Conectado a {puerto} a {baudios} baudios")
    print("Presiona Ctrl+C para salir\n")
    
    while True:
        if ser.in_waiting > 0:
            linea = ser.readline().decode('utf-8', errors='ignore').strip()
            if linea:
                print(linea)
except serial.SerialException as e:
    print(f"Error: {e}")
    print(f"\nVerifica que el puerto {puerto} sea correcto.")
    print("Puertos disponibles: ejecuta 'python -m serial.tools.list_ports' para verlos")
except KeyboardInterrupt:
    print("\nDesconectado")
    ser.close()
