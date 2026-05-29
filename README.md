Version 3 elm327 OBD2

Tabla completa de PIDs OBD-II (Modo 01)
PID	Descripción	Bytes	Fórmula
0100	PIDs soportados 01-20	4	bitmask
0101	Estado MIL y monitores	4	bits
0102	Freeze DTC	2	hex
0103	Sistema de combustible	2	enum
0104	Carga calculada del motor	A	A × 100 / 255
0105	Temperatura refrigerante	A	A − 40
0106	STFT Bank1	A	(A − 128) / 1.28
0107	LTFT Bank1	A	(A − 128) / 1.28
0108	STFT Bank2	A	(A − 128) / 1.28
0109	LTFT Bank2	A	(A − 128) / 1.28
010A	Presión combustible	A	A × 3 kPa
010B	Presión MAP	A	A kPa
010C	RPM motor	A B	(256A + B) / 4
010D	Velocidad vehículo	A	km/h
010E	Avance encendido	A	(A/2) − 64
010F	Temp aire admisión	A	A − 40
0110	Flujo MAF	A B	(256A + B) / 100 g/s
0111	Posición acelerador	A	A × 100 / 255
0112	Aire secundario	A	enum
0113	Estado sensores O2	A	bitmask
Sensores de oxígeno (los que ya estás usando)
PID	Sensor	Fórmula
0114	O2 B1S1	Voltaje A×0.005V / Trim (B−128)/1.28
0115	O2 B2S1	Voltaje A×0.005V / Trim (B−128)/1.28
0116	O2 B1S2	Voltaje A×0.005V / Trim (B−128)/1.28
0117	O2 B2S2	Voltaje A×0.005V / Trim (B−128)/1.28
0118	O2 B1S3	igual
0119	O2 B2S3	igual
011A	O2 B1S4	igual
011B	O2 B2S4	igual
Más PIDs importantes
PID	Descripción	Fórmula
011C	OBD estándar	enum
011D	Sensor O2 presentes	bitmask
011E	Aux input status	bits
011F	Run time motor	(256A+B) s
Combustible y emisiones
PID	Descripción	Fórmula
0120	PIDs soportados 21-40	bitmask
0121	Distancia con MIL	(256A+B) km
0122	Presión rail	(256A+B)×0.079 kPa
0123	Presión rail diesel	(256A+B)×10
0124	O2 wideband B1S1	ver estándar
0125	O2 wideband B2S1	
0126	O2 wideband B1S2	
0127	O2 wideband B2S2	
Temperaturas y sensores
PID	Descripción	Fórmula
012C	EGR command	A×100/255
012D	Error EGR	(A−128)/1.28
012E	EVAP purge	A×100/255
012F	Nivel combustible	A×100/255
0130	Warmups desde borrado	A
0131	Distancia desde borrado	(256A+B) km
0132	Presión EVAP	(256A+B)/4 Pa
0133	Presión barométrica	A kPa
Sensores adicionales
PID	Descripción	Fórmula
0134	O2 B1S1 wideband	estándar
0135	O2 B1S2 wideband	
0136	O2 B1S3 wideband	
0137	O2 B1S4 wideband	
Control del motor
PID	Descripción	Fórmula
0140	PIDs soportados 41-60	bitmask
0142	Voltaje módulo control	(256A+B)/1000 V
0143	Carga absoluta	(256A+B)/255
0144	Relación aire combustible	(256A+B)/32768
0145	Posición acelerador relativa	A×100/255
# obd2
