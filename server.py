#ibrerias necesarias para el correcto funcionamiento del servidor
from flask import Flask, render_template, jsonify, request
import sqlite3
import sys
import json
import hashlib
import codecs
from itertools import cycle

#Método que toma una cadena de caracteres y les aplica la operación XOR 
def str_xor(s1, s2):
 return ''.join(chr(ord(c)^ord(k)) for (c,k) in zip(s1, cycle(s2)))


#Ruta de la base de datos de los sensores y GPS
db_path = 'sensor_database/sensorData.db'

#Se inicializa un servidor Flask
app = Flask(__name__)

#Se renderiza el index.html 
@app.route('/')
def home():
    return render_template('index.html')
#Se renderiza el registro.html 
@app.route('/registro')
def registro():
    return render_template('registro.html')
#Se elimina la base de datos de los sensores
@app.route('/borrardb')
def borrardb():
    con = sqlite3.connect(db_path)
    cur = con.cursor()
    cur.execute("DROP TABLE IF EXISTS data")
    cur.execute("CREATE TABLE data (idsensor NUMERIC, timestamp DATETIME, temp NUMERIC, lat NUMERIC, lon NUMERIC)")
    con.commit()
    con.close()
    return render_template('index.html')

#Se renderiza el chart.html que contiene todas las visualizaciones de los datos censados enviados por el end device almacenados en la base de datos
@app.route("/visualizacion")
def captura():
    sensorID = []
    sensorTime = []
    sensorTemp = []
    sensorHum = []
    SensorLux = []
    sensorLat = []
    sensorLon = []
    con = sqlite3.connect(db_path)
    curs = con.cursor()
    for fila in curs.execute("SELECT * FROM data"): #Consulta SQL para obtener los datos almacenados en la base de datos
        sensorID.append(fila[0])
        sensorTime.append(fila[1])
        sensorTemp.append(fila[2])
        sensorHum.append(fila[3])
        sensorLux.append(fila[4])
        sensorLat.append(fila[5])
        sensorLon.append(fila[6])
    con.close()
    leyenda = 'Sensor de temperatura', 'Sensor de humedad', 'Sensor de radiación solar'
    etiquetas = sensorTime
    return render_template('chart.html', temperaturas = sensorTemp, humedades = sensorHum, labels=etiquetas, legend=leyenda)

#Se captura los datos, procesan y validan 
@app.route('/sensor_send_data', methods=['POST'])
def sensor_send():

    key = "-/-///-//-" #Llave para desencriptar XOR
    data = request.get_json() #Se obtienen los datos del end device
    valuesconverted = json.loads(data["data"]) #Se convierten el mensaje a JSON
    datahash = hashlib.sha1(data[("data")].encode('utf-8')).hexdigest() #Se codifican los datos con el algoritmo sha1()

    data["checksum"] = data["checksum"][0:len(data["checksum"])-1] #Se elimina el ultimo caracter al valor de la llave data["checksum"] ya que no se requiere

    res = str_xor(datahash,key) #Se decodifica el datahash con la operación XOR y obtener el string de los bytes del mensaje
    decode = ":".join("{:02x}".format(ord(c)) for c in res) #Se le da formato hexadecimal al resultado de la operación anterior

    if(decode.strip() == data["checksum"].strip()): #Se compara el resultado del checksum con el anterior para validar el no repudio e integridad de los datos, si es válido se insertan los datos a la bd, en caso contrario los rechaza

        print("Eso es todo")
        print(valuesconverted)
        print(valuesconverted["temp"])
        print(valuesconverted["hum"])
        print(valuesconverted["lux"])
        print(valuesconverted["lat"])
        print(valuesconverted["lon"])
        print(valuesconverted["alt"])
        con = sqlite3.connect(db_path)
        cur = con.cursor()
        #Se inserta a la base de datos los datos censados y enviados por el end device
        cur.execute("INSERT INTO data VALUES(" + str(data["id"]) + "," + "datetime('now')," + str(valuesconverted["temp"]) + "," + str(valuesconverted["hum"]) + "," + str(valuesconverted["lux"]) + "," + str(valuesconverted["lat"]) + "," + str(valuesconverted["lon"]) + "," + str(valuesconverted["alt"]) + ")")
        con.commit()
        con.close()

    else: 
        print("Error")

    return "ok",201

#Pruebas en la recepción de los datos    
@app.route("/datos", methods = ['POST'])
def datos():

    key = "-/-///-//-"
    data = request.get_json()
    valuesconverted = json.loads(data["data"])
    datahash = hashlib.sha1(data[("data")].encode('utf-8')).hexdigest()
    #if datahash == data["checksum"]:
     #   print("hola")
      #  datadata = json.loads(data["data"])
      #  print(datadata["temp"])

    data["checksum"] = data["checksum"][0:len(data["checksum"])-1]
    res = str_xor(datahash,key)
    decode = ":".join("{:02x}".format(ord(c)) for c in res)

    print(decode.strip())
    print(data["checksum"].strip())

    if(decode.strip() == data["checksum"].strip()):
        #print(data["id"])
        #print(valuesconverted)
        print("Envia datos a la bd")
    else: 
        print("Eso no es todo")
    return 'Todo ok'

if __name__ == '__main__':
        #Se crean y agregan el certificado y llave para la conexión segura con SSL
        app.run(debug=True,host='0.0.0.0',port=443, ssl_context=('cert.crt', 'key.pem'))