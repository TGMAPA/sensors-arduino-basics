#include <Servo.h>
#include <LiquidCrystal.h>
#include <DHT.h>


// Distribución de pines
const short bombilla=12, led_cortina=8, led_calefaccion=7;

//Fotoresistor
const short solar = 0;

// Sensor humedad y temp
const short sensor_hd_temp = 6; // pin para sensor de humedad/temp
DHT dht(sensor_hd_temp, DHT11);

// Servomotor Cortina
const short cortina=9;   // pin para servomotor  
Servo servo_cortina; //Instancia de servomotor

// Servomotor Cortina
const short bomba_agua=10;   // pin para servomotor  
Servo servo_bomba_agua; //Instancia de servomotor

// LCD Display 
LiquidCrystal lcd(13, 11, 5, 4, 3, 2); // pines para LCD (RS, EN, d4, d5, d6, d7)

// Declaracion de variables condiciones del invernadero
unsigned int luz=0, luz_ant=0, luz_display=0, t=0;
bool bom=false, cor=false, calefaccion=false, b_agua=false;
float temperatura= 0.0, humedad= 0.0;

//    C.I.
unsigned int intensidad=200, fotoperiodo=12;
float humedad_ideal= 30.0, temp_ideal= 20.0;
// fotoperiodo = cantidad de tiempo que deberia estar expuesta la planta a la luz

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {

  //LCD setup
  lcd.begin(16, 3);
  
  // Inicio de comunicación serial
  Serial.begin(9600);
  
  // Inicio de sensor 
  dht.begin();

  // Pin setup
  pinMode(solar, INPUT);
  pinMode(cortina,OUTPUT);
  pinMode(led_cortina, OUTPUT);
  pinMode(bombilla,OUTPUT);
  pinMode(led_calefaccion, OUTPUT);

  // Servo Cortina setup a posición inicial de 0°
  servo_cortina.attach(cortina);
  servo_cortina.write(0);
  delay(1000);

  // Servo Bomba de agua setup a posición inicial de 0° --> Cerrada
  servo_bomba_agua.attach(bomba_agua);
  servo_bomba_agua.write(0);
  delay(1000);

  // ENCABEZADO PARA TERMINAL VIRTUAL (COMUNICACIÓN SERIAL)
  Serial.println("===============================================");  
  Serial.println("Comunicacion serial -- Invernadero automatizado "); 
  Serial.println("==============================================="); 
  Serial.println();
  Serial.println("  Iniciando sistema...");
  delay(1500);
  Serial.println("  SISTEMA LISTO PARA OPERAR");
  Serial.println();
  Serial.println("Mensaje  - Descripcion");
  Serial.println("------------------------------------------------------------------------------------------------");
}


void loop() {
  //Lectura de señal Analoga --> Fotoresistor --> Luz solar
  luz=analogRead(solar);
  luz_display= (luz*100)/407;
  
  // Lectura de humedad
  humedad = dht.readHumidity();

  //Lectura de temperatura
  temperatura = dht.readTemperature();

  // Impresión de condiciones ambientales en LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Luz     = ");
  lcd.print(luz_display);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Humedad = ");
  lcd.print(humedad);
  lcd.print("%");
  lcd.setCursor(0, 2);
  lcd.print("Temp    = ");
  lcd.print(temperatura);
  lcd.print(" C");
  
  
  // Control de la bomba de agua de acuerdo con la humedad
  if(humedad < humedad_ideal && !b_agua){
    Serial.println("[Aviso]  Humedad baja");
    Serial.println("[Evento] Activando bomba de agua...");
    servo_bomba_agua.write(90);
    delay(500);
    Serial.println("[Aviso]  Bomba de agua activa");
    b_agua=true;
  }
  if(humedad >= humedad_ideal && b_agua){
    delay(5000);
    servo_bomba_agua.write(0);
    delay(500);
    Serial.println("[Aviso]  Humedad controlada");
    Serial.println("[Evento] Bomba de agua desactivada");
    b_agua=false;
  }

  
  // Control de temperatura para calefacción
  if(temperatura < temp_ideal && !calefaccion){
    Serial.println("[Aviso]  Temperatura baja");
    Serial.println("[Evento] Activando Calefaccion...");
    digitalWrite(led_calefaccion, HIGH);
    Serial.println("[Aviso]  Calefaccion activa");
    calefaccion=true;
  }
  if(temperatura > temp_ideal+5 && !calefaccion){
    Serial.println("[Aviso]  Temperatura alta");
    Serial.println("[Evento] Activando Calefaccion...");
    digitalWrite(led_calefaccion, HIGH);
    Serial.println("[Aviso]  Calefaccion activa");
    digitalWrite(led_calefaccion, HIGH);
    calefaccion=true;
  }
  if((temperatura >= temp_ideal && temperatura <= temp_ideal+5) && calefaccion){
    Serial.println("[Aviso]  Temperatura controlada");
    Serial.println("[Evento] Calefaccion desactivada");
    digitalWrite(led_calefaccion, LOW);
    calefaccion=false;
  }
    
 

  // Control de la cubierta para la luz solar y el suplemento de luz (bombilla)
  if(luz >= intensidad && luz_ant < intensidad || luz_ant>=intensidad && luz<intensidad ) t=0;
  luz_ant=luz;

  if(luz<intensidad){
    if(cor){
      cor=false;
      Serial.println("[Aviso]  Luz solar inferior a la intensidad requerida");
      Serial.println("[Evento] Desactivando Cubierta...");
      servo_cortina.write(0);
      digitalWrite(led_cortina,LOW);
      delay(500);
      t=0;
    }
    if(t>(24-fotoperiodo) && !bom){
      Serial.println("[Aviso]  Tiempo sin exposicion solar excedido");
      Serial.println("[Evento] Activando Iluminacion suplementaria...");
      digitalWrite(bombilla,HIGH);
      t=0;
      bom=true;
    }
    if(t>=fotoperiodo && bom){  // bombilla ya estuvo encendida el tiempo necesario 
      Serial.println("[Aviso]  Tiempo de iluminacion suplementaria excedido ");
      Serial.println("[Evento] Desactivando Iluminacion suplementaria...");
      digitalWrite(bombilla,LOW);
      bom=false;
      t=0;
    }
  }

  if(luz>=intensidad){
    if(bom && (!cor || (cor && t >= 24-fotoperiodo))){   // Apagar bombilla porque la cubierta no desplegada (porque hay luz)
      bom=false;    
      Serial.println("[Aviso]  Luz solar suficiente, Iluminación suplementaria inecesaria ");
      Serial.println("[Evento] Desactivando Iluminacion suplementaria...");                                    
      digitalWrite(bombilla,LOW);
      t=0;
    }
    if((t>fotoperiodo || luz>intensidad+100) && !cor) { // Si el tiempo es mayor al fotoperiodo y la luz es más intensa de lo establecido --> desplegar la cortina
      Serial.println("[Aviso]  Tiempo de Fotoperiodo excedido o intensidad de luz solar extrema ");
      Serial.println("[Evento] Activando Cubierta...");  
      servo_cortina.write(180);
      digitalWrite(led_cortina,HIGH);
      delay(500);
      cor=true;
      t=0;
    }
    if (t >= 24-fotoperiodo && cor){ // Tiempo sin exposición solar excedido 
      Serial.println("[Aviso]  Tiempo sin exposicion solar excedido, Iluminacion suplementaria necesaria ");
      Serial.println("[Evento] Activando Iluminacion suplementaria..."); 
      digitalWrite(bombilla,HIGH);
      bom=true;
      t=0;
    }
    if((t >= 24-fotoperiodo && luz < intensidad+100 || luz<intensidad) && cor){ // Si el tiempo es mayor al tiempo que no tiene que estar expuesto a la luz y la luz no es demasiado intensa, o si laluz es menor a la intensidad y la cubierta esta puesta --> Cerrar la cubierta
      
      Serial.println("[Aviso]  Tiempo sin exposicion solar excedido o intensidad de luz solar regular ");
      Serial.println("[Evento] Desactivando Cubierta...");  
      servo_cortina.write(0);
      digitalWrite(led_cortina,LOW);
      delay(500);
      cor=false;
      t=0;
    }
  }
  delay(1000);
  t++;
}

