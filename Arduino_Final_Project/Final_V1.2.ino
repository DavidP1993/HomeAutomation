//David Petter. 
//November 2016, project Home Automation.



//************************************************************************************************************************************************************************
//GLOBAL INCLUDES (LIBARIES):
// http://185.13.227.161/phpmyadmin     //IP of the SQL overview website.
#include <Ethernet.h>                   //Ethernet connection libary
#include <MySQL_Connection.h>           //SQL
#include <MySQL_Cursor.h>               //SQL
#include <Wire.h>                       //T.b.v. I2C
#include <LiquidCrystal_I2C.h>          //I2C
#include "DHT.h"                        //DHT_11 sensor



//************************************************************************************************************************************************************************
//SQL & IP DATA:
byte mac_addr[] = { 0x90, 0xA2, 0xDA, 0x10, 0x1D, 0xA5 };       //Arduno MAC adress
IPAddress server_addr(185, 13, 227, 161);                       //IP of the MySQL *server* here        185.13.227.161
char user[] = "hanspot144_hanze";                               // MySQL user login username            hanspot144_hanze
char password[] = "Hanze2016";                                  // MySQL user login password            Hanze2016
EthernetClient client;                                          // Ethernetclient name: client.
MySQL_Connection conn((Client *)&client);                       // Mysql connection with ethernetclient instance.
LiquidCrystal_I2C lcd(0x27, 20, 4);                             // I2C adress & screen size.



//************************************************************************************************************************************************************************
//Gloabal declarations (needed in more than one function):
unsigned long VorigeTijd = 0;          //Geheugenplaats om wachttijd bij te houden van het ethernet verbdinden delay-deel

bool LcdShowState = LOW;               //Hiermee wordt gecontroleerd welke tekst wordt weergegeven op het LCD scherm (schakelt tussen twee weergaven).
bool ZeroRan = LOW;                    //Tevens voor controle van tekst van LCD.
bool OneRan = HIGH;                    //Tevens voor controle van tekst van LCD.

unsigned long LcdVorigetijd = 0;       //Geheugenplaats om wachttijd bij te houden van het weergeven van info op LCD.

unsigned long HistoryVorigeTijd = 0;   //Geheugenplaats om wachttijd bij te houden, HistoryVorigeTijd is een timer voor het uploaden van meetwaarden in DB.

const int  FanUpButton = 22;          //Button pin van vocht omhoog
int FanUpButtonState = 0;             //Current state of the button
int FanUpButtonlastButtonState = 0;   //Previous state of the button

const int FanDownButton = 24;         //Button pin van vocht omlaag
int FanDownButtonState = 0;           //current state of the button
int FanDownButtonlastButtonState = 0; //previous state of the button

const int HumUpButton = 28;           //Button pin fan omhoog
int HumUpButtonState = 0;             //Current state of the button
int HumUpButtonlastButtonState = 0;   //previous state of the button

const int HumDownButton = 30;         //Button pin fam omlaag
int HumDownButtonState = 0;           //Current state of the button
int HumDownButtonlastButtonState = 0; //Previous state of the button

const int Led1 = 50;            //LED rood links
const int Led2 = 52;            //LED rood rechts

const int HumTempSensor = 40;   //Vocht sensor
const int FlowSensor1 = A8;     //Flow sensor windsnelheid
const int FlowSensor2 = A9;     //Flow sensor temperatuur
const int CO2Sensor = A10;      //CO2 sensor
const int FanOutPin = 44;       //PWM MOSFET

float CO2Setpoint;              //Geheuhenlocatie voor setpoint
float OldCO2Setpoint;           //"Oude" setpoint, wordt gebruikt om te kijken of het setpoint gewijzigd is.
int HumSetpoint;                //Idem
int OldHumSetpoint;             //Idem
int FanSetpoint;                //Idem
int OldFanSetpoint;             //Idem

int HumReading = 0;          //Ingelezen waarden vanaf sensoren
int TempReading = 0;         //Idem
int CO2Reading = 0;          //Idem
float FlowReading = 0.0;     //Idem
int FlowIntReading = 0;      //FlowReading wordt omgezet naar integer.

int LastChanged = 0;          //Waarde om te kijken welk setpoint als laatst gewijzigd is. 0 = regelen op vocht; 1 = regelen op fan.
bool AreWeConnected = 0;      //Waarde om te testen of er een Ethernetverbinding is.
bool StandAloneEnable = 0;    //Waarde om status van Standalonemodus aan te geven. (true betekent geen verb.) Default 0.
float DC_GAIN = 6.5;          //Versterking van de CO2 sensor
DHT dht(HumTempSensor, DHT11);    //Instance van DHT11 sensor opstarten. Met naam dht.



//************************************************************************************************************************************************************************
//Setup code (runs just once at startup):
void setup() {
  lcd.init();                       //Initialize the lcd
  lcd.backlight();                  //Backlight on
  lcd.clear();                      //LCD leegmaken

  pinMode(FanUpButton, INPUT);      //Pin als input configureren
  pinMode(FanDownButton, INPUT);    //Idem
  pinMode(HumUpButton, INPUT);      //Idem
  pinMode(HumDownButton, INPUT);    //Idem
  //Analoge inputs van sensoren worden niet gedeclareerd.

  pinMode(Led1, OUTPUT);            //Pin als output configureren
  pinMode(Led2, OUTPUT);            //Idem

  lcd.print("LED test");            //LED's testen bij optarten.
  digitalWrite(Led1, HIGH);
  delay(500);
  digitalWrite(Led1, LOW);
  digitalWrite(Led2, HIGH);
  delay(500);
  digitalWrite(Led2, LOW);
  lcd.clear();

  CO2Setpoint = 600;  //Initiele setpoints vaststellen
  HumSetpoint = 40;   //Idem
  FanSetpoint = 1;    //Idem
  WriteValues();      //Eerste keer ventilator aansturen bij opstarten.

  Connect();          //Verbinden met DB.
  dht.begin();        //Sensor instance opstarten. Wordt later opgeroepen met dht.xxxxx.
}   //END SETUP();



//************************************************************************************************************************************************************************
//Loop code, runs indefinitely.
void loop() {

  if (StandAloneEnable == 0) {        //Als de ethernetverbinding goed is, gaan we naar connect. StandAloneEnable is default 0.
    Connect();                        //Verbinden met DB (DataBase).
  }   //END IF;
  else if (StandAloneEnable == 1) {   //Anders naar Standalone.
    Standalone();
  }   //END ELSE;

}   //END LOOP();



//************************************************************************************************************************************************************************
//Connect() function; Used to make a connection with te DB.
void Connect() {

  if (AreWeConnected == 0) {                //Als er geen verbinding met ethernet is
    lcd.clear();                            //LCD leegmaken
    lcd.print("Trying to connect");         //Show
    if (Ethernet.begin(mac_addr) == 0) {    //Probeer verbinden met DHCP, als dat niet lukt...
      Standalone();                         //Gaan we naar de standalone function.
      StandAloneEnable = 1;                 //Standalone boolean bijwerken.
      AreWeConnected = 0;                   //Connected boolean (blijft) 0.
    }

    else if (Ethernet.begin(mac_addr) == 1) {                 //Als er echter wél een verbinding met ethernet is gemaakt
      lcd.clear();                                            //LCD leegmaken
      lcd.print("Connecting...");                             //Show
      if (conn.connect(server_addr, 3306, user, password)) {  //Gaan we verbinding maken met de DB server
        lcd.setCursor(0, 1);
        lcd.print("Connected !");
        delay(1000);                                          //1 sec wachten
        lcd.clear();
        AreWeConnected = 1;                                   //Connected boolean bijwerken, we willen 1 want de verbinding is gelukt
        StandAloneEnable = 0;                                 //Standalone boolean bijwerken, wordt 0 want we willen nu geen standalone
        OnlineMode();                                         //Naar de online function
      }
    }
  } else if (AreWeConnected == 1) {                           //Als er reeds een verbinding was
    OnlineMode();                                             //Gaan we direct door naar de online function.
  }

}   //END CONNECT();



//************************************************************************************************************************************************************************
//Standalone() function; Used when there is no connecton with ethernet and database.
void Standalone() {

  const long WachtTijd = 60000;                   //Wachttijd voordat verbinden opnieuw wordt geprobeerd. = 1 minuut.
  const long LcdWachttijd = 5000;                 //Wachttijd voordat lcd andere info laat zien (schakelen tussen 2 schermen). (5 sec).
 
  unsigned long HuidigeTijd = millis();           //Systeemtijd van Arduino ophalen.
  if (HuidigeTijd - VorigeTijd >= WachtTijd) {    //Als wachttijd verstreken is
    VorigeTijd = HuidigeTijd;                     //Wordt de vorige tijd gelijk aan de wachttijd
    StandAloneEnable = 0;                         //En gaan we uit de Standalone modus. Default 0. In de Loop() function zal opnieuw verbinding worden gemaakt.
    AreWeConnected = 0;                           //Maar de boolean Areweconnected blijft 0, aangezien we (nog) geen verbdinding hebben.
  }

  HuidigeTijd = millis();                             //Systeemtijd van Arduino ophalen.
  if (HuidigeTijd - LcdVorigetijd >= LcdWachttijd) {  //Als wachttijd verstreken is
    LcdVorigetijd = HuidigeTijd;                      //Wordt de vorige tijd gelijk aan de wachttijd
    if (LcdShowState == 0)                            //Kijken welke teskt reeds weergegeven is
    {
      ZeroRan = 0;                                    //En op basis daarvan een keuze maken.
      LcdShowState = 1;                               //Deze boolean geeft aan welke tekst de LCD moet weergeven
    }
    else if (LcdShowState == 1)                       
    {
      OneRan = 0;                                    //Als ShowState 1 is, wordt het tweede deel van de tekst op LCD weergegeven.
      LcdShowState = 0;
    }
  }   //END IF.

  //Weergavedeel nul
  if (ZeroRan == LOW) {                   //Afhankelijk van LcdShowState in alinea hierboven
    lcd.clear();
    lcd.setCursor(0, 0);                  //Cursor naar positie nul, regel nul.
    lcd.print("- Standalone modus -");

    lcd.setCursor(0, 1);
    lcd.print("CO2 nu: ");
    lcd.print(CO2Reading);                //Waarde tonen op LCD
    lcd.print("PPM");

    lcd.setCursor(0, 2);
    lcd.print("Flow nu: ");
    lcd.print(FlowReading);
    lcd.print("M/s");

    if (LastChanged == 0) {               //Hier wordt op de onderste regel getoond in welke regelmodus de Arduino staat (vocht of fan).
      lcd.setCursor(0, 3);                //Dat is afhankelijk van de boolean "LastChanged". 0 is vocht, 1 is fan.
      lcd.print("Regel op vocht");
      lcd.print("(");
      lcd.print(HumSetpoint);
      lcd.print("%");
      lcd.print(")");
    } else if (LastChanged == 1) {
      lcd.setCursor(0, 3);
      lcd.print("Regelen op fan ");
      lcd.print(FanSetpoint);
    }

    Regelen();                            //Iedere 5sec regelen, zodat bij een verandering van vocht het FanSetpoint opnieuw berekend wordt.
    ZeroRan = HIGH;                       //Deze boolean hoog maken, zodat de andere tekst (OneRan) wordt weergegeven op LCD.
  }   //END ZERORAN.
  
  //Weergavedeel een
  if (OneRan == LOW) {                    //Afhankelijk van LcdShowState in alinea hierboven
    lcd.clear();
    lcd.setCursor(0, 0);                  //Cursor naar positie nul, regel nul.
    lcd.print("- Standalone modus -");

    lcd.setCursor(0, 1);
    lcd.print("Luchtvocht: ");
    lcd.print(HumReading);
    lcd.print("%");

    lcd.setCursor(0, 2);
    lcd.print("Temperatuur: ");
    lcd.print(TempReading);
    lcd.print(" C");

    if (LastChanged == 0) {
      lcd.setCursor(0, 3);
      lcd.print("Regel op vocht");
      lcd.print("(");
      lcd.print(HumSetpoint);
      lcd.print("%");
      lcd.print(")");
    } else if (LastChanged == 1) {
      lcd.setCursor(0, 3);
      lcd.print("Regelen op fan ");
      lcd.print(FanSetpoint);
    }

    OneRan = HIGH;
  }   //END ONERAN

  ReadInputs();       //Inputs uitlezen via analoge ingangen.
  ReadButtons();      //Functie ReadButtons() aanroepen om schakelaars in te lezen.
}   //END STANDALONE().



//************************************************************************************************************************************************************************
//Online() function; Used when the connection with ethernet & DB are okay.
void OnlineMode() {
  
  const long HistoryWachttijd = 60000;                      //Interval om history in DB te zetten (30 seconden).
  const long LcdWachttijd = 5000;                           //Wachttijd voordat lcd andere info laat zien (schakelen tussen 2 schermen). (5 sec).
  
  unsigned long DezeTijd = millis();                        //Systeemtijd van Arduino ophalen.
  if (DezeTijd - HistoryVorigeTijd >= HistoryWachttijd) {   //Als wachttijd verstreken is.
    HistoryVorigeTijd = DezeTijd;                           //History in de DB zetten, alle meetwaarden worden gelogd.
    InsertHistory();                                        //Iedere 30sec deze funnctie oproepen (meetwaarden in DB zetten).
  }

  unsigned long HuidigeTijd = millis();                     //Systeemtijd van Arduino ophalen. 
  if (HuidigeTijd - LcdVorigetijd >= LcdWachttijd) {        //Als wachttijd verstreken is
    LcdVorigetijd = HuidigeTijd;
    if (LcdShowState == 0)                                  //Kijken welke teskt reeds weergegeven is
    {                                                       //Dit is hetzelfde als bij de StandAlone() modus.
      ZeroRan = 0;                                          
      LcdShowState = 1;
    }
    else if (LcdShowState == 1)
    {
      OneRan = 0;
      LcdShowState = 0;
    }
  }   //END IF.                               

  if (ZeroRan == LOW) {                             //Afhankelijk van LcdShowState in alinea hierboven
    lcd.clear();
    lcd.setCursor(0, 0);                            //Cursor naar positie nul, regel nul.
    lcd.print("-   Online modus   -");

    lcd.setCursor(0, 1);
    lcd.print("CO2 nu: ");
    lcd.print(CO2Reading);
    lcd.print("PPM");

    lcd.setCursor(0, 2);
    lcd.print("Flow nu: ");
    lcd.print(FlowReading);
    lcd.print("M/s");

    if (LastChanged == 0) {                         //Allemaal hetzelde als bij StandAlone().
      lcd.setCursor(0, 3);
      lcd.print("Regel op vocht");
      lcd.print("(");
      lcd.print(HumSetpoint);
      lcd.print("%");
      lcd.print(")");
    } else if (LastChanged == 1) {
      lcd.setCursor(0, 3);
      lcd.print("Regelen op fan ");
      lcd.print(FanSetpoint);
    }

    OldHumSetpoint = HumSetpoint;                             //Elke 5sec
    OldCO2Setpoint = CO2Setpoint;                             //Elke 5sec
    OldFanSetpoint = FanSetpoint;                             //Elke 5sec
    GetFanSetpoint(); //Voor ventilator  setpont ophalen      //Elke 5sec
    GetSetpoints();   //Voor de vocht en CO2 setpoint ophalen //Elke 5sec
    Regelen();                                                //Elke 5sec

    ZeroRan = HIGH;
  } //END ZERORAN


  if (OneRan == LOW) {                    //Afhankelijk van LcdShowState in alinea hierboven
    lcd.clear();
    lcd.setCursor(0, 0);                  //Cursor naar positie nul, regel nul.
    lcd.print("-   Online modus   -");

    lcd.setCursor(0, 1);
    lcd.print("Luchtvocht: ");
    lcd.print(HumReading);
    lcd.print("%");

    lcd.setCursor(0, 2);
    lcd.print("Temperatuur: ");
    lcd.print(TempReading);
    lcd.print(" C");

    if (LastChanged == 0) {
      lcd.setCursor(0, 3);
      lcd.print("Regel op vocht");
      lcd.print("(");
      lcd.print(HumSetpoint);
      lcd.print("%");
      lcd.print(")");
    } else if (LastChanged == 1) {
      lcd.setCursor(0, 3);
      lcd.print("Regelen op fan ");
      lcd.print(FanSetpoint);
    }
    OneRan = HIGH;
  } //END OneRan.


  ReadInputs();                           //Meetwaarden van sensoren ophalen.
  ReadButtonsSQL();                       //Buttons uitlezen in SQL(online) mode, anders dan bij StandAlone().
}   //END ONLINEMODE();



//************************************************************************************************************************************************************************
//InsertHistory() function; Used to insert sensor values into the DB at given intervals.
void InsertHistory() {      //Met deze functie zetten we data in de DB.
  
  int IdNumber = 1;         //Insert gebeurt met ID1, wij zijn groep 1.
  int KlepStand = 0;        //Ons project maakt geen gebruik van een klep, een klepstand hebben we dus niet.

  char INSERT[] = "INSERT INTO hanspot144_hanze.History (id, CO2, Humidity, ExtraSensor, KlepStand, Temp) VALUES(%d,%d,%d,%d,%d,%d)";   //DB naam, tabelnaam, namen van kolommen en type values (%d is int).
  char query[128];    //De SQL cursor gebruikt 128 karakters om de data naar de DB te sturen.    

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);    //Cursor aanmaken met geheugenpointer *cur_mem.

  sprintf(query, INSERT, IdNumber, CO2Reading, HumReading, FlowIntReading, KlepStand, TempReading);   //Deze waarden zijn de actuele weetwaarden.        
  cur_mem->execute(query);    //Query uitvoeren met cursor. 
  delete cur_mem;             //Geheugen weer leegmaken

}   //END INSERTHISTORY().



//************************************************************************************************************************************************************************
//GetSetpoints() function; Used to get two setpoints (humid and CO2) from the DB.
void GetSetpoints() {
  
  char GetSetpoints[] = "SELECT HumiditySetpoint, CO2Setpoint FROM hanspot144_hanze.SetPoints WHERE id=1";      //Setpoints voor deze node ophalen
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);      //Cursor aanmaken met geheugenpointer *cur_mem.
  cur_mem->execute(GetSetpoints);                       //Query uitvoeren met cursor. 
  column_names *cols = cur_mem->get_columns();          //Kolommen ophalen die de cursor heeft gevonden.

  row_values *row = NULL;                 
  do {
    row = cur_mem->get_next_row();                      //Rijen ophalen uit de kolom
    if (row != NULL) {
      HumSetpoint = atol(row->values[0]);               //Op plek 0 zit het vocht setpoint
      CO2Setpoint = atol(row->values[1]);               //Op plek 1 zit het CO2 setpont
    }
  } while (row != NULL);
  delete cur_mem;                                       //Geheugen weer leegmaken

  if (HumSetpoint != OldHumSetpoint) {                  //Als het opgehaalde setpoint verschilt van het huidige setpoint.
    lcd.clear();
    lcd.print("- Nieuw  setpoint: -");
    lcd.setCursor(0, 1);
    lcd.print("vanuit database");
    lcd.setCursor(0, 2);
    lcd.print(HumSetpoint);                             //Nieuwe setpint weergeven op scherm.
    lcd.print(" %");
    LcdVorigetijd = (millis() - 3500);                  //LCD weergavetijd verzetten zodat nieuwe setpoint 1,5 sec wordt weergegeven.
    LastChanged = 0;                                    //Bijhouden welke waarde het laatst gewijzigd is. 0 = vocht, 1 = fan.
    Regelen();                                          //Fucntie regelen oproepen, hier wordt gekeken of op fan-stand of vocht wordt geregeld.
  }

  if (CO2Setpoint != OldCO2Setpoint) {                  //Zelfde als hierboven, maar dan voor CO2.
    lcd.clear();                                        //We regelen echter niet op CO2, dus we doen er verder niets mee.
    lcd.print("- Nieuw  setpoint: -");                  //Aangezien regelen op CO2 zeer vreemd is. (vinden wij).
    lcd.setCursor(0, 1);
    lcd.print("vanuit database");
    lcd.setCursor(0, 2);
    lcd.print(CO2Setpoint);
    lcd.print(" PPM");
    LcdVorigetijd = (millis() - 3500);
  }

}   //END GETSETPOINTS().



//************************************************************************************************************************************************************************
//GetFanSetpoint() function; Used to get the fan setpoint. (this is under a different ID number, so it is a seperate function.
void GetFanSetpoint() {
  
  char GetSetpoints[] = "SELECT HumiditySetpoint, CO2Setpoint FROM hanspot144_hanze.SetPoints WHERE id=0";      //Setpoint voor ventilator ophalen
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(GetSetpoints);
  column_names *cols = cur_mem->get_columns();

  row_values *row = NULL;                       //Alles is hier precies hetzelfde als bij GetSetpoints.
  do {                                          //Alleen het ID nummer is "0", omdat daar het fan setpoint in zit.
    row = cur_mem->get_next_row();
    if (row != NULL) {
      FanSetpoint = atol(row->values[1]);       //Het fansetpoint zit in de kolom van "CO2SetPoint", dat is kolom 1.
    }
  } while (row != NULL);
  delete cur_mem;

  if (FanSetpoint != OldFanSetpoint) {          //Als het opgehaalde setpoint verschilt van het huidige setpoint.
    lcd.clear();
    lcd.print("- Nieuw  setpoint: -");
    lcd.setCursor(0, 1);
    lcd.print("vanuit database");
    lcd.setCursor(0, 2);
    lcd.print("stand ");
    lcd.print(FanSetpoint);
    LcdVorigetijd = (millis() - 3500);
    LastChanged = 1;                            //Bijhouden welke waarde het laatst gewijzigd is. 0 = vocht, 1 = fan.
    Regelen();                                  //Fucntie regelen oproepen, hier wordt gekeken of op fan-stand of vocht wordt geregeld.
  }

}   //END GETFANSETPOINT().



//************************************************************************************************************************************************************************
//ReadInputs() function; Used to read the sensor values from our four sensors.
void ReadInputs() {   //Functie om de sensorwaarden in te lezen.

//Deze #define waarden zijn specifiek voor deze CO2 sensor, en zijn gecalibreerd:
#define         ZERO_POINT_X                 (2.602)  //lg400=2.602, the start point_on X_axis of the curve
#define         ZERO_POINT_VOLTAGE           (0.324)  //define the output of the sensor in volts when the concentration of CO2 is 400PPM
#define         MAX_POINT_VOLTAGE            (0.265)  //define the output of the sensor in volts when the concentration of CO2 is 10,000PPM
#define         REACTION_VOLTGAE             (0.059)  //define the voltage drop of the sensor when move the sensor from air into 1000ppm CO2
  float           CO2Curve[3]  =  {ZERO_POINT_X, ZERO_POINT_VOLTAGE, (REACTION_VOLTGAE / (2.602 - 4))};   
  CO2Reading = CO2GetPercentage(CO2Curve);            //Functie om de daadwerkelijke CO2 waarde op te halen.

//Dit is voor vocht & tenp:
  HumReading = dht.readHumidity();                    //De acuele vochtwaarde zit in de dht instance met naam dht.
  TempReading = dht.readTemperature();                //De acuele temp zit in de dht instance met naam dht.

//En dit is allemaal voor de windsnelheid:
  const float zeroWindAdjustment =  .25;  //Gecalibreerde waarde gevonden door windstil meting uit te voeren.
  int TMP_Therm_ADunits;                  //Temperatuurwaarde van windsensor
  float RV_Wind_ADunits;                  //RV output from wind sensor
  float RV_Wind_Volts;                    //Voltage output
  int TempCtimes100;                      //Opgegeven door fabrikant
  float zeroWind_ADunits;
  float zeroWind_volts;
  float temp = 0.0;                       //Tijdelijke waarde om de flowmeting te vermenigvuldigen
  TMP_Therm_ADunits = analogRead(FlowSensor1);                                              //WindSpeed (uncalibrated)
  RV_Wind_ADunits = analogRead(FlowSensor2);                                                //Temperature drop (uncalibrated)
  RV_Wind_Volts = (RV_Wind_ADunits *  0.0048828125);                                        //Al deze calibratiewaarden zijn opgegeven door de fabrikant, en moeten niet gewijzigd worden.
  TempCtimes100 = (0.005 * ((float)TMP_Therm_ADunits * (float)TMP_Therm_ADunits)) - (16.862 * (float)TMP_Therm_ADunits) + 9075.4;   //Idem
  zeroWind_ADunits = -0.0006 * ((float)TMP_Therm_ADunits * (float)TMP_Therm_ADunits) + 1.0727 * (float)TMP_Therm_ADunits + 47.172;  //Idem 
  zeroWind_volts = (zeroWind_ADunits * 0.0048828125) - zeroWindAdjustment;                                                          //Idem
  FlowReading =  (pow(((RV_Wind_Volts - zeroWind_volts) / .2300) , 2.7265) * 0.44704);      //Dit is de flowreading in meter per seconde (float) gecalibreerd.
  temp = (FlowReading * 100);                                                               //De meting wordt 100 keer groter gemaakt.
  FlowIntReading = (int) temp;                                                              //En dan omgezet naar een inreger, dit doen we omdat de SQL geen floats kan versturen
}   //END READINPUTS();



//************************************************************************************************************************************************************************
//CO2GetPercentage() function; Used to get the actual CO2 percentage.
int  CO2GetPercentage(float *pcurve) {
  
  float volts;        //Voltage divided by amplifier gain (defined in ReadInputs() ).
  float v = 0;        //Measured voltage value.

  v += analogRead(CO2Sensor);
  v = ((v * 5) / 1024);

  volts = v / DC_GAIN;
  if (volts > ZERO_POINT_VOLTAGE || volts < MAX_POINT_VOLTAGE ) {       //Als gemeten waarde niet tussen defined max/min waarden is
    return -1;                                                          //Geeft de sensor "-1" terug.
  } else {
    return pow(10, (volts - pcurve[1]) / pcurve[2] + pcurve[0]);        //Anders wordt er aan de hand van een CO2 curve een waarde berekend.
    volts = 0;                                                          //Wordt weer "0" gemaakt  voor de volgende meting.
  }
  delete pcurve;                                                        //Geheugen weer leegmaken.
  
}   //END CO2GETPERCENTAGE();



//************************************************************************************************************************************************************************
//ReadButtons() function; Used to read the buttons in OFFLINE mode.
void ReadButtons() {

  //FAN OMHOOG
  FanUpButtonState = digitalRead(FanUpButton);                  //De status wordt iedere keer ingelezen                        
  if (FanUpButtonState != FanUpButtonlastButtonState) {         //Als er een verandering is in de waarde (0 naar 1, of vice versa)
    if (FanUpButtonState == LOW) {                              //Schakelaar is actief laag, hier wordt nogmaals gekeken of de schakelaar is ingedrukt.
      lcd.clear();
      lcd.print("-  Fan setpoint:  -");                         //Info op LCD tonen
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(OldFanSetpoint);                                //Huidige setpoint tonen
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      FanSetpoint = (OldFanSetpoint + 1);                       //Nieuwe setpoint wordt met één verhoogd
      ShowFanError();                                           //Functie oproepen waar wordt getest of het nieuwe setpoint binnen juiste bereik valt.
      lcd.print(FanSetpoint);
      OldFanSetpoint = FanSetpoint;                             //De geheugenplaats voor het oude setpoint wordt bijgewerkt naar het nieuwe setpoint
      LcdVorigetijd = (millis() - 3500);                        //Wachttijd van weergave scherm aanpassen, zodat na 1,5 sec het nieuwe setpoint weer uit scherm verdwijnt.
      LastChanged = 1;                                          //Bijhouden welke waarde het laatst gewijzigd is. 1 = fan, 0 = vocht.
      Regelen();                                                //De regelen() functie oproepen om de ventilator aan te sturen
    }
    delay(50);                                                  //Debounce om dubbel tellen te voorkomen.
  }
  FanUpButtonlastButtonState = FanUpButtonState;                //Schakelaarstatus wordt bijgewerkt.

  //FAN OMLAAG
  FanDownButtonState = digitalRead(FanDownButton);              //Methode gelijk aan FAN OMHOOG
  if (FanDownButtonState != FanDownButtonlastButtonState) {
    if (FanDownButtonState == LOW) {
      lcd.clear();
      lcd.print("-  Fan setpoint:  -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(OldFanSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      FanSetpoint = (OldFanSetpoint - 1);                       //Alleen wordt het setpoint hier met één verlaagd.
      ShowFanError();                                           //Functie oproepen waar wordt getest of het nieuwe setpoint binnen juiste bereik valt.
      lcd.print(FanSetpoint);
      OldFanSetpoint = FanSetpoint;
      LcdVorigetijd = (millis() - 3500);
      LastChanged = 1;
      Regelen();
    }
    delay(50);
  }
  FanDownButtonlastButtonState = FanDownButtonState;

  //VOCHT OMHOOG                                                //Methode gelijk aan FAN OMHOOG / FAN OMLAAG
  HumUpButtonState = digitalRead(HumUpButton);
  if (HumUpButtonState != HumUpButtonlastButtonState) {
    if (HumUpButtonState == LOW) {
      lcd.clear();
      lcd.print("- Humid. Setpoint: -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(OldHumSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      HumSetpoint = RoundToFive((OldHumSetpoint + 5));          //Vocht wordt met 5 verhoogd, omdat er een spectrum is van 100.
      ShowHumError();                                           //Functie oproepen waar wordt getest of het nieuwe setpoint binnen juiste bereik valt.
      lcd.print(HumSetpoint);                                   //RoundToFive is een functie die zorgt dat het setpoint wordt afgerond naar 5.
      lcd.print(" %");                                          //Dus 22 wordt 20, en 23 wordt 25.
      OldHumSetpoint = HumSetpoint;
      LcdVorigetijd = (millis() - 3500);
      LastChanged = 0;                                          //De boolean wordt nu 0 omdat we gaan regelen op een vocht setpoint.
      Regelen();
    }
    delay(50);
  }
  HumUpButtonlastButtonState = HumUpButtonState;

  //VOCHT OMLAAG
  HumDownButtonState = digitalRead(HumDownButton);              //Methode gelijk aan FAN OMHOOG / FAN OMLAAG / VOCHT OMHOOG
  if (HumDownButtonState != HumDownButtonlastButtonState) {
    if (HumDownButtonState == LOW) {
      lcd.clear();
      lcd.print("- Humid. Setpoint: -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(OldHumSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      HumSetpoint = RoundToFive((OldHumSetpoint - 5));
      ShowHumError();
      lcd.print(HumSetpoint);
      lcd.print(" %");
      OldHumSetpoint = HumSetpoint;
      LcdVorigetijd = (millis() - 3500);
      LastChanged = 0;
      Regelen();
    }
    delay(50);
  }
  HumDownButtonlastButtonState = HumDownButtonState;
  
}   //END READBUTTONS();



//************************************************************************************************************************************************************************
//ReadButtonsSQL() function; Used to read the buttons in ONLINE mode, when there is a connection to the DB.                   //In principe is deze functie precies gelijk aan de functie ReadButtons().
void ReadButtonsSQL() {                                                                                                       //Alleen wordt er hier gebruik gemaakt van een functie waarmee de setpoints
                                                                                                                              //in een aparte functie (UpdateFanSetpoint & UpdateSetpoints) worden verhoogd.
  //FAN OMHOOG                                                                                                                //Daarom wordt deze functie alleen aangeroepen in de ONLINE modus.
  FanUpButtonState = digitalRead(FanUpButton);              //De status wordt iedere keer ingelezen
  if (FanUpButtonState != FanUpButtonlastButtonState) {     //Hier wordt schakelaar verandering gedetecteerd.
    if (FanUpButtonState == LOW) {                          //Schakelaar is actief laag (hier wordt hij ingedrukt).
      lcd.clear();
      lcd.print("-  Fan setpoint:  -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(FanSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      lcd.print(FanSetpoint = (FanSetpoint + 1));
      ShowFanError();
      Regelen();                                            //De Regelen() functie oproepen. Hier wordt geregeld o.b.v. fan- of vocht setpoint.
      UpdateFanSetpoint();                                  //En daarna de UpdateFanSetpoint() functie aanroepen, waar het nieuwe setpoint in de DB wordt geschreven.
      LcdVorigetijd = (millis() - 3500);                    //Wachttijd van weergave scherm aanpassen, zodat na 1,5 sec het nieuwe setpoint weer uit scherm verdwijnt.
      LastChanged = 1;                                      //Bijhouden welke waarde het laatst gewijzigd is.
    }
    delay(50);                                              //Debounce om dubbel tellen te voorkomen.
  }
  FanUpButtonlastButtonState = FanUpButtonState;            //Schakelaarstatus wordt bijgewerkt.

  //FAN OMLAAG
  FanDownButtonState = digitalRead(FanDownButton);          //Methode gelijk aan FAN OMHOOG     
  if (FanDownButtonState != FanDownButtonlastButtonState) {
    if (FanDownButtonState == LOW) {
      lcd.clear();
      lcd.print("-  Fan setpoint:  -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(FanSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      lcd.print(FanSetpoint = (FanSetpoint - 1));
      ShowFanError();
      Regelen();
      UpdateFanSetpoint();
      LcdVorigetijd = (millis() - 3500);
      LastChanged = 1;
    }
    delay(50);
  }
  FanDownButtonlastButtonState = FanDownButtonState;

  //VOCHT OMHOOG                                            //Methode gelijk aan FAN OMHOOG / FAN OMLAAG
  HumUpButtonState = digitalRead(HumUpButton);
  if (HumUpButtonState != HumUpButtonlastButtonState) {
    if (HumUpButtonState == LOW) {
      lcd.clear();
      lcd.print("- Humid. Setpoint: -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(OldHumSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      HumSetpoint = RoundToFive((OldHumSetpoint + 5));      //RoundToFive is een functie die zorgt dat het setpoint wordt afgerond naar 5.
      OldHumSetpoint = HumSetpoint;                         //Dus 22 wordt 20, en 23 wordt 25.
      ShowHumError();
      lcd.print(HumSetpoint);
      lcd.print(" %");
      Regelen();
      UpdateSetpoints();
      LcdVorigetijd = (millis() - 3500);
      LastChanged = 0; 
    }
    delay(50);
  }
  HumUpButtonlastButtonState = HumUpButtonState;

  //VOCHT OMLAAG                                            //Methode gelijk aan FAN OMHOOG / FAN OMLAAG / VOCHT OMHOOG
  HumDownButtonState = digitalRead(HumDownButton);
  if (HumDownButtonState != HumDownButtonlastButtonState) {
    if (HumDownButtonState == LOW) {
      lcd.clear();
      lcd.print("- Humid. Setpoint: -");
      lcd.setCursor(0, 1);
      lcd.print("Current setpoint: ");
      lcd.print(HumSetpoint);
      lcd.setCursor(0, 2);
      lcd.print("New setpoint: ");
      HumSetpoint = RoundToFive((OldHumSetpoint - 5));
      OldHumSetpoint = HumSetpoint;
      ShowHumError();
      lcd.print(HumSetpoint);
      lcd.print(" %");
      Regelen();
      UpdateSetpoints();
      LcdVorigetijd = (millis() - 3500);  
      LastChanged = 0;  
    }
    delay(50);
  }
  HumDownButtonlastButtonState = HumDownButtonState;
  
}   //END ReadButtonsSQL();



//************************************************************************************************************************************************************************
//UpdateFanSetpoint() function; Used to write a new FAN setpoint to the DB when the user has changed the setpoint locally.
void UpdateFanSetpoint() {
  
  int IdNumber = 0;                                                                         //Juiste kolom aanwijzen in de DB, wij zijn groep 1 maar het fan setpoint zit op ID_0.  
  char UPDATE[] = "UPDATE hanspot144_hanze.SetPoints SET  CO2Setpoint='%d' WHERE id=%d";    //De SetPoints tabel aanwijzen, waar het setpoint in de kolom "CO2Setpoint" moet.
  char query[128];                                                                          //De SQL cursor gebruikt 128 karakters om de data naar de DB te sturen. 

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);                                          //Cursor aanmaken met geheugenpointer *cur_mem.
  sprintf(query, UPDATE, FanSetpoint, IdNumber);                                            //"Tekst" aanmaken die als query naar de SQL server wordt gestuurd.
  cur_mem->execute(query);                                                                  //De query uitvoeren
  delete cur_mem;                                                                           //Geheugen weer leegmaken
  
}   //END UpdateFanSetpoint();



//************************************************************************************************************************************************************************
//UpdateSetpoints() function; Used to write new setpoints of humidity and CO2 to the DB when the user has changed the setpoints locally.
void UpdateSetpoints() {
  
  int IdNumber = 1;                                                                           //Juiste kolom aanwijzen in de DB, wij zijn groep 1
  char UPDATE[] = "UPDATE hanspot144_hanze.SetPoints SET  HumiditySetpoint='%d' WHERE id=%d"; //Voor de rest is deze functie gelijk aan UpdateFanSetpoint().
  char query[128];

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  sprintf(query, UPDATE, HumSetpoint, IdNumber);
  cur_mem->execute(query);
  delete cur_mem;
  
}   //END UpdateSetpoints();



//************************************************************************************************************************************************************************
//ShowFanError() function; Used to give user an error if setpoints are given outside of range
void ShowFanError() {    //Error tonen als ongeldinge combinatie is ingevoerd.

  if (FanSetpoint < 0 || FanSetpoint > 3){
  lcd.clear();
  lcd.print("Kies fan instelling");
  lcd.setCursor(0, 1);
  lcd.print("tussen 0 en 3");
  lcd.setCursor(0, 2);
  if (FanSetpoint <= 0) {           //Als waarde onder 0 wordt gekozen
    FanSetpoint = 0;                //Moet het 0 worden.
  } else {                            //Als waarde hoger dan 3 wordt gekozen
    FanSetpoint = 3;               //Moet het 3 worden.
  }
  LcdVorigetijd = (millis() - 3500);                    //Wachttijd van weergave scherm aanpassen, zodat na 1,5 sec het nieuwe setpoint weer uit scherm verdwijnt.
}

}   //END ShowFanError();



//************************************************************************************************************************************************************************
//ShowHumError() function; Used to give user an error if setpoints are given outside of range
void ShowHumError() {     //Error tonen als ongeldinge combinatie is ingevoerd.

  if (HumSetpoint < 0 || HumSetpoint > 100){
  lcd.clear();
  lcd.print("Kies vochtinstelling");
  lcd.setCursor(0, 1);
  lcd.print("tussen 0 en 100%.");
  if (HumSetpoint <= 0) {             //Als waarde onder 0 wordt gekozen
    HumSetpoint = 0;                  //Moet het 0 worden.
  } else {                            //Als waarde hoger dan 100 wordt gekozen
    HumSetpoint = 100;                //Moet het 100 worden.
  }
  LcdVorigetijd = (millis() - 3500);                    //Wachttijd van weergave scherm aanpassen, zodat na 1,5 sec het nieuwe setpoint weer uit scherm verdwijnt.
}

}   //END ShowHumError();



//************************************************************************************************************************************************************************
//Regelen() function; Used to calculate the fan setting, either by direct fan setpoint, or by calculating based on moisture reading and setpoint.
void Regelen() {
  
  int Set;          //Variable to store the setpoint
  int Current;      //Variable to store the current value (reading)
  int Offset;       //Variable calculated by this function
  int FanSpeed;     //Actual setting to control te fan; 0, 1, 2, 3

  switch (LastChanged) {                      //LastChanged is de waarde die veranderd wordt bij het wijzigen van een setpoint
    case 0:                                   //Bij 0 moet er worden geregeld op vocht setpoint, bij 1 wordt er geregeld op fan setpoint
      Set = HumSetpoint;                      //Waarde inlezen
      Current = HumReading;                   //Idem
      Offset = (Current - Set);               //Bij negatef moet fan langzamer, bij positief moet fan sneller.
      FanSpeed = map(Offset, -100, 100, 0, 3);//Hiermee wordt voor de offset een fan instelling tussen 0 en 3 bepaald.
      FanSetpoint = FanSpeed;                 //Het fansetpoint is dan die waarde
      OldFanSetpoint = FanSetpoint;           //Het "oude" setpoint wordt aan de nieuwe gelijk gemaakt, zodat elders in de software geen verandering wordt gedetecteerd.

      if (StandAloneEnable == 0 && AreWeConnected == 1) {         //Als er een DB verbinding is
        UpdateFanSetpoint();                                      //Zorgen dat de DB van de verandering op de hoogte is.
      } else if (StandAloneEnable == 1 || AreWeConnected == 0) {  //Bij StandAlone() kan dit niet en mag dit ook niet, anders loopt de microcontroller vast
        break;                                                    //Dus gaan we dan uit deze IF
      }
      
      WriteValues();                          //Fan aansturen.
      break;                                  //Uit case(0)

    case 1:                                   //Bij LastChanged = 1 wordt er direct op fan setpoint geregeld, en is de waarde (0, 1, 2 of 3) al bekend
      WriteValues();                          //Dus gaan we gelijk de fan aansturen
      break;                                  //En uit de case(1)
  }

}   //END Regelen();



//************************************************************************************************************************************************************************
//WriteValues() function; Used to compute the fan setting (0, 1, 2 or 3) to a PWM value and write to the fan pin.
void WriteValues() {
  
  int PWMValue;                     //The computed PWM value is stored here

  switch (FanSetpoint) {            //FanSetpoint is 0, 1, 2 of 3
    case 0:
      PWMValue = 0;                 //Bij 0 is de fan uit
      break;

    case 1:
      PWMValue = 85;                //Bij 1 is de PWM-waarde 85
      break;

    case 2:
      PWMValue = 170;               //Bij 2 is de PWM-waarde 170
      break;

    case 3:
      PWMValue = 255;               //Bij 3 is de PWM-waarde 255 (maximale powerrr)
      break;

    default:
      PWMValue = 85;                //Als er een andere waarde (niet tussen 0 en 3) wordt gelezen, schrijven we 85 (is 33%).
  }
  analogWrite(FanOutPin, PWMValue); //Uiteindelijk schrijven we de berekende PWM-waarde naar de digitale uitgang.
     
}   //END WriteValues();



//************************************************************************************************************************************************************************
//RoundToFive() function; Used to round a given int to the nearest five: 22 becomes 20, 23 becomes 25 etc.
int RoundToFive(int n) {
  return (n/5 + (n%5>2)) * 5;
}   //END RoundToFive();



//END OF SOFTWARE PROGRAM.
//David Petter. 
//November 2016, project Home Automation.
