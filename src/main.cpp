#include <Arduino.h>
#include <math.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <vector>
#include <esp_sleep.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
#define SCREEN_ADDRESS 0x3C

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12
#define SERVOMO 18
#define LDR 36

// create an OLED display object connected to I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHTesp dhtSensor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Servo servo;

// Global variables
int days=0;
int hours=0;
int minutes=0;
int seconds=0;
int temp_hour=0;
int temp_minute=0;

bool alarms_enabled[]={false,false};
int n_alarms=2;
int alarm_hours[]={0,0};
int alarm_minutes[]={0,0};
bool alarm_triggered[]={false,false};

int n_notes=8;
int C=262;
int D=294;
int E=330;
int F=349;
int G=392;
int A=440;
int B=494;
int C_H=523;
int notes[]={C,D,E,F,G,A,B,C_H};

int current_mode=0;
int max_modes=5;
String modes[]={"1 - Set Time Zone","2 - Set Alarm 1","3 - Set Alarm 2","4 - View Active Alarms","5 - Delete a alarm"};

int state = 1; //state of the mc

std::vector<float> ldr_readings;
unsigned long sampling_interval = 5000; // in milliseconds
unsigned long sending_interval = 120000; // in milliseconds
unsigned long last_sample_time = 0;
unsigned long last_average_time = 0;

float minimum_angle=30.00;
float controlling_factor=0.75;
float ideal_temp=30.00;

void print_line(String text,int column,int row,int text_size)
{
  display.setTextSize(text_size);         // set text size
  display.setTextColor(SSD1306_WHITE);    // set text color
  display.setCursor(column, row);       // set position to display (x,y)
  display.println(text); // set text
  display.display();  
}

void recieveCallback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length + 1];
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  payloadCharAr[length] = '\0';

  if (strcmp(topic, "switch-on-off") == 0)
  {
    state = atoi(payloadCharAr);
    Serial.println(state);
  }
  if(strcmp(topic, "get-sampling-time") == 0)
  {
    sampling_interval = strtoul(payloadCharAr, NULL, 10); // <-- type conversion
    sampling_interval = sampling_interval*1000;
    Serial.println(sampling_interval);
  }
  if(strcmp(topic, "get-sending-time") == 0)
  {
    sending_interval = strtoul(payloadCharAr, NULL, 10); // <-- type conversion
    sending_interval = sending_interval*60*1000;
    Serial.println(sending_interval);
  }
  if(strcmp(topic, "get-ideal-temp") == 0)
  {
    ideal_temp = atof(payloadCharAr);
    Serial.println(ideal_temp);
  }
  if(strcmp(topic, "get-controlling-factor") == 0)
  {
    controlling_factor = atof(payloadCharAr);
    Serial.println(controlling_factor);
  }
  if(strcmp(topic, "get-minimum-angle") == 0)
  {
    minimum_angle = atof(payloadCharAr);
    Serial.println(minimum_angle);
  }
  if(strcmp(topic, "get-alarm-1") == 0)
  {
    if(payloadCharAr[0] == '1')
    {
      alarms_enabled[0]=true;
    }
    else if(payloadCharAr[0] == '0')
    {
      alarms_enabled[0]=false;
    }
  }
  if(strcmp(topic, "get-alarm-2") == 0)
  {
    if(payloadCharAr[0] == '1')
    {
      alarms_enabled[1]=true;
    }
    else if(payloadCharAr[0] == '0')
    {
      alarms_enabled[1]=false;
    }
  }
}

void setupMqtt() {
  mqttClient.setServer("broker.hivemq.com", 1883);
  mqttClient.setCallback(recieveCallback);
}

void setup() {

  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(LDR,INPUT);

  dhtSensor.setup(DHTPIN,DHTesp::DHT22);
  servo.attach(SERVOMO);

  Serial.begin(9600);

  // initialize OLED display with I2C address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    for (;;);
  }

  display.display();
  delay(2000);         // wait two seconds for initializing

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI",0,0,2);
  }

  display.clearDisplay(); // clear display
  print_line("Connected to WIFI",0,0,2);

  setupMqtt();

  display.clearDisplay();
  print_line("Welcome to Medibox!",10,20,2);
  delay(2000);
  display.clearDisplay();

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  print_line("Press ok to goto menu",0,30,1);
  delay(500);
}

void print_time_now()
{
  display.clearDisplay();
  print_line(String(days),0,0,2);
  print_line(":",20,0,2);
  print_line(String(hours),30,0,2);
  print_line(":",50,0,2);
  print_line(String(minutes),60,0,2);
  print_line(":",80,0,2);
  print_line(String(seconds),90,0,2);
  print_line("Press ok to goto menu",0,30,1);
}

void update_time()
{
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  hours=atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  minutes=atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  seconds=atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3, "%H", &timeinfo);
  days=atoi(timeDay);

  hours=hours+temp_hour;
  if(hours>23)
  {
    hours=hours%24;
    days++;
  }

  minutes=minutes+temp_minute;
  if(minutes>59)
  {
    minutes=minutes%60;
    hours++;
  }
}

void ring_alarm()
{
  display.clearDisplay();
  print_line("MEDICINE TIME!",0,0,2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  int ringing_time=(int)millis();
  
  while(break_happened == false && digitalRead(PB_CANCEL)==HIGH && ringing_time/1000<=300)
  {
  for (int i=0;i<n_notes;i++)
  {
    if (digitalRead(PB_CANCEL)==LOW)
    {
      delay(200);
      break_happened=true;
      break;
    }
    tone(BUZZER,notes[i]);
    delay(500);
    noTone(BUZZER);
    delay(2);
  }
  delay(10000);
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm()
{
   update_time();
   print_time_now();

  
      for(int i=0;i<n_alarms;i++)
      {
        if (alarms_enabled[i] == true && alarm_triggered[i] == false && alarm_hours[i]==hours && alarm_minutes[i]==minutes)
        {
          ring_alarm();
          alarm_triggered[i]=true;
        }
      }
  
}

int wait_for_button_press()
{
  while(true)
  {
    if(digitalRead(PB_UP) == LOW)
    {
      delay(200);
      return PB_UP;
    }

     else if(digitalRead(PB_DOWN) == LOW)
    {
      delay(200);
      return PB_DOWN;
    }

    else if(digitalRead(PB_OK) == LOW)
    {
      delay(200);
      return PB_OK;
    }

    else if(digitalRead(PB_CANCEL) == LOW)
    {
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}

void set_offset_time()
{
  while(true)
  {
  display.clearDisplay();
  print_line("Enter offset hours: "+String(temp_hour),0,0,1);

  int pressed=wait_for_button_press();

    if(pressed == PB_UP)
    {
      delay(200);
      temp_hour+=1;
      temp_hour=temp_hour % 24;
    }
    else if(pressed == PB_DOWN)
    {
      delay(200);
      temp_hour-=1;
      if(temp_hour<0)
      {
        temp_hour=23;
      }
    }
      else if(pressed == PB_OK)
    {
      delay(200);
      break;
    }
       else if(pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  while(true)
  {
  display.clearDisplay();
  print_line("Enter offset minutes: "+String(temp_minute),0,0,1);

  int pressed=wait_for_button_press();

    if(pressed == PB_UP)
    {
      delay(200);
      temp_minute+=1;
      temp_minute=temp_minute % 60;
    }
    else if(pressed == PB_DOWN)
    {
      delay(200);
      temp_minute-=1;
      if(temp_minute<0)
      {
        temp_minute=59;
      }
    }
      else if(pressed == PB_OK)
    {
      delay(200);
      break;
    }
       else if(pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }
 
  display.clearDisplay();
  print_line("Offset is set",0,0,2);
  delay(1000);
}

void set_alarm(int alarm)
{
  int temp_hour=alarm_hours[alarm];

  while(true)
  {
  display.clearDisplay();
  print_line("Enter hour: "+String(temp_hour),0,0,1);

  int pressed=wait_for_button_press();

    if(pressed == PB_UP)
    {
      delay(200);
      temp_hour+=1;
      temp_hour=temp_hour % 24;
    }
    else if(pressed == PB_DOWN)
    {
      delay(200);
      temp_hour-=1;
      if(temp_hour<0)
      {
        temp_hour=23;
      }
    }
      else if(pressed == PB_OK)
    {
      delay(200);
      alarm_hours[alarm]=temp_hour;
      break;
    }
       else if(pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }

  int temp_minute=alarm_minutes[alarm];

  while(true)
  {
  display.clearDisplay();
  print_line("Enter minute: "+String(temp_minute),0,0,1);

  int pressed=wait_for_button_press();

    if(pressed == PB_UP)
    {
      delay(200);
      temp_minute+=1;
      temp_minute=temp_minute % 60;
    }
    else if(pressed == PB_DOWN)
    {
      delay(200);
      temp_minute-=1;
      if(temp_minute<0)
      {
        temp_minute=59;
      }
    }
      else if(pressed == PB_OK)
    {
      delay(200);
      alarm_minutes[alarm]=temp_minute;
      break;
    }
       else if(pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }
 
  display.clearDisplay();
  alarms_enabled[alarm]=true;
  print_line("Alarm is set",0,0,2);
  delay(1000);
}

void view_active_alarms()
{
   int row=0;
   display.clearDisplay();
   for(int i=0;i<n_alarms;i++)
   {
      if(alarms_enabled[i]==true)
      {
        print_line("Alarm "+String(i+1)+" enabled",0,row,1);
        row+=30;
      }
   }
   delay(1000);
}

void delete_alarm()
{
  int alarm=1;
  display.clearDisplay();
  for(int i=0;i<n_alarms;i++)
  {
    print_line("Delete alarm "+String(alarm)+" ?",0,0,2);
    int pressed=wait_for_button_press();

    if(pressed == PB_CANCEL)
    {
      alarm++;
      continue;
    }
    else if(pressed==PB_OK)
    {
      if(i==0)
      {
        alarms_enabled[0]=alarms_enabled[1];
        alarm_hours[0]=alarm_hours[1];
        alarm_minutes[0]=alarm_minutes[1];
        alarm_triggered[0]=alarm_triggered[1];
        n_alarms=n_alarms-1;
      }
      if(i==1)
      {
        n_alarms=n_alarms-1;
      }
    }
  }
}

void check_temp()
{
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  if(data.temperature>32)
  {
    // display.clearDisplay();
    print_line("TEMP NOT HEALTHY",0,40,1);
  }
  else if(data.temperature<24)
  {
    // display.clearDisplay();
    print_line("TEMP NOT HEALTHY",0,40,1);
  }

  if(data.humidity>80)
  {
    // display.clearDisplay();
    print_line("HUMIDITY NOT HEALTHY",0,50,1);
  }
  else if(data.humidity<65)
  {
    // display.clearDisplay();
    print_line("HUMIDITY NOT HEALTHY",0,50,1);
  }
}

void run_mode(int mode)
{
  if(mode==0)
  {
    set_offset_time();
  }
  else if(mode ==1 || mode ==2)
  {
    set_alarm(mode-1);
  }
  else if(mode==3)
  {
    view_active_alarms();
  }
  else if(mode==4)
  {
    delete_alarm();
  }
}

void go_to_menu()
{
  while(digitalRead(PB_CANCEL) == HIGH)
  {
    display.clearDisplay();
    print_line("Displaying current mode...",0,0,1);
    print_line(modes[current_mode],0,30,1);

    int pressed=wait_for_button_press();

    if(pressed == PB_UP)
    {
      delay(200);
      current_mode+=1;
      current_mode=current_mode % max_modes;
    }
    else if(pressed == PB_DOWN)
    {
      delay(200);
      current_mode-=1;
      if(current_mode<0)
      {
        current_mode=max_modes-1;
      }
      
    }
    else if(pressed == PB_OK)
    {
      delay(200);
      run_mode(current_mode);
    }
    else if(pressed == PB_CANCEL)
    {
      delay(200);
      break;
    }
  }
}

void connectToBroker() {
  while(!mqttClient.connected()){
    Serial.println("Attempting MQTT connection");
    if(mqttClient.connect("ESP32-12345645454")){
      Serial.println("connected");
      mqttClient.subscribe("switch-on-off");
      mqttClient.subscribe("get-sampling-time");
      mqttClient.subscribe("get-sending-time");
      mqttClient.subscribe("get-ideal-temp");
      mqttClient.subscribe("get-controlling-factor");
      mqttClient.subscribe("get-minimum-angle");
      mqttClient.subscribe("get-alarm-1");
      mqttClient.subscribe("get-alarm-2");
    }else{
      Serial.println("failed");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void serv_mo(float ldr_reading){
  
  TempAndHumidity data = dhtSensor.getTempAndHumidity();

  float theta = minimum_angle + (180-minimum_angle)*ldr_reading*controlling_factor*log((float)sampling_interval / (float)sending_interval)*(data.temperature/ideal_temp);
  Serial.print("Servo angle: ");
  Serial.println(theta);
  servo.write(theta);
}

void read_LDR()
{
  float ldr_value = analogRead(LDR)*1.00;
  ldr_value =(ldr_value - 4063.00) / (32.00 - 4063.00);
  Serial.print("LDR Value: ");
  Serial.println(ldr_value);
  ldr_readings.push_back(ldr_value);
  serv_mo(ldr_value);
}

void loop() 
{
  check_temp();
  delay(200);

  if(!mqttClient.connected()){
    Serial.println("Reconnecting to MQTT Broker");
    connectToBroker();
  }
  mqttClient.loop();
    
  mqttClient.publish("temp", String(dhtSensor.getTemperature(), 2).c_str());
  mqttClient.publish("humidity", String(dhtSensor.getHumidity(), 2).c_str());

  update_time_with_check_alarm();
  if(digitalRead(PB_OK) == LOW)
  {
    delay(200);
    go_to_menu();
  }

  unsigned long now = millis();

  // Take LDR reading every sampling_interval
  if(now - last_sample_time >= sampling_interval)
  {
    last_sample_time = now;
    Serial.println(now);
    read_LDR();
  }

  // Average and publish every sending_interval
  if(now - last_average_time >= sending_interval)
  {
    last_average_time = now;
    if(!ldr_readings.empty()) {
      float sum = 0.00;
      for(int i=0; i<ldr_readings.size(); i++)
      {
        sum += ldr_readings[i];
      }
      float avg = sum / ldr_readings.size();
      Serial.println(avg);
      mqttClient.publish("LDR", String(avg).c_str());
      mqttClient.publish("ldr-update",(String(hours)+":"+String(minutes)+":"+String(seconds)).c_str());
      ldr_readings.clear();
    }
  }

  if(state==0)
  {
    esp_deep_sleep(10*1000000);
  }

  if(alarms_enabled[0]==true)
  {
    mqttClient.publish("set-alarm-1",(String("Enabled")).c_str());
  }
  else{
    mqttClient.publish("set-alarm-1",(String("Disabled")).c_str());
  }

  if(alarms_enabled[1]==true)
  {
    mqttClient.publish("set-alarm-2",(String("Enabled")).c_str());
  }
  else{
    mqttClient.publish("set-alarm-2",(String("Disabled")).c_str());
  }

  mqttClient.publish("time-update",(String(hours)+":"+String(minutes)+":"+String(seconds)).c_str());

  delay(200);
}