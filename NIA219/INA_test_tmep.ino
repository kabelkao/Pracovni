/* Poznámky:
- pro funční čtení napětí baterie je nutné kompilovat kód verzí knihovny 2.0.9 pro desku ESP32 C3 Dev Module

- padá do brownoutu
*/

// ============= NAPĚTÍ BATERKY ============= //
  #include <Wire.h>
  #include <SPI.h>
  #include <ESP32AnalogRead.h>

  ESP32AnalogRead adc;
  #define ADCpin 0
  #define DeviderRatio 1.7693877551  // Delic napeti z akumulatoru 1MOhm + 1.3MOhm
  
  float bat_voltage = 0;

  void baterkaF()
  {
    bat_voltage = adc.readVoltage() * DeviderRatio;
    Serial.println();
    Serial.print("Napětí na baterii = " );
    Serial.print(bat_voltage);
    Serial.println(" V");
  }
// ============= NAPĚTÍ BATERKY ============= //

// ============= INA219 ============= //
  #include <Wire.h>
  #include <Adafruit_INA219.h>

  Adafruit_INA219 ina219; // Použita defaultní adresa čidla (0x40)

  #define SDA 19
  #define SCL 18
  #define PIN_ON 3

  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  float power_mW = 0;

  void inaF()
  {
    Serial.println("Measuring voltage and current with INA219 ...");
    Serial.println();

    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    current_mA = ina219.getCurrent_mA();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    
    Serial.print("Bus Voltage:   "); Serial.print(busvoltage); Serial.println(" V");
    Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
    Serial.print("Current:       "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
    Serial.println("");
  }
// ============= INA219 ============= //

// ============= SHT40 ============= //
  #include <Wire.h>
  #include "Adafruit_SHT4x.h"
  Adafruit_SHT4x sht4 = Adafruit_SHT4x();
  
  float tempSHT40 = 0;
  float humSHT40 = 0; 

  void dataSHT40()
  {
    sensors_event_t humidity, temp; // temperature and humidity variables
    sht4.getEvent(&humidity, &temp);

    Serial.println();
    Serial.print("Teplota vzduchu = ");
    tempSHT40 = temp.temperature;
    Serial.print(tempSHT40);
    Serial.println(" °C");
    
    Serial.print("Vlhkost vzduchu = ");
    humSHT40 = humidity.relative_humidity;
    Serial.print(humSHT40);
    Serial.println(" %");
  }
// ============= SHT40 ============= //

// ============= WIFI+TMEP ============= //
  #include <WiFi.h>
  #include <esp_wifi.h>
  #include <HTTPClient.h>

  const char* ssid = "Kopretina"; //Wifi nova; Kopretina; Vodafone-F59F
  const char* password = "SvataKaterina"; //ajrad1961; SvataKaterina; MWk8anwctn8urmve

  String serverName = "http://epkpug-awv3v4.tmep.cz/index.php?";

  void wifiF()
  {
    Serial.println();
    Serial.print("WiFi = ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dB");

    if(WiFi.status()== WL_CONNECTED)
    {
        HTTPClient http;

        //GUID
        String serverPath = serverName 
        + "loadV=" + loadvoltage
        + "&curr=" + current_mA
        + "&pwr=" + power_mW
        + "&v=" + bat_voltage
        + "&rssi=" + WiFi.RSSI()
        + "&tempSHT40=" + tempSHT40
        + "&humSHT40=" + humSHT40;

        Serial.println(serverPath);
        
        // zacatek http spojeni
        http.begin(serverPath.c_str());
        
        // http get request
        int httpResponseCode = http.GET();
        
        if (httpResponseCode>0) 
        {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println(payload);
        }
        else 
        {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }
        // Uvolneni
        http.end();
    }
    else 
    {
      Serial.println("Wi-Fi odpojeno");
    }
  }
// ============= WIFI+TMEP ============= //



void setup(void) 
{
  Serial.begin(115200);
  // ============= NAPĚTÍ BATERKY ============= //
    adc.attach(ADCpin);
  // ============= NAPĚTÍ BATERKY ============= //

  // ============= I2C ============= //
    Wire.begin(SDA, SCL);
  // ============= I2C ============= //
  
  // ============= WIFI+TMEP ============= //
    int pokusWifi = 0; // promena pro pocitani pokusu o pripojeni

    WiFi.begin(ssid, password);
    Serial.println("Pripojovani");
    while(WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
      if(pokusWifi > 20) // Pokud se behem 10s nepripoji, uspi se na 300s = 5min
      {
        esp_sleep_enable_timer_wakeup(300 * 1000000);
        esp_deep_sleep_start();
      }
      pokusWifi++;
    }

    Serial.println();
    Serial.print("Pripojeno do site, IP adresa zarizeni: ");
    Serial.println(WiFi.localIP());
  // ============= WIFI+TMEP ============= //
  
  // ============= INA219 ============= //
    pinMode(PIN_ON, OUTPUT);      // Set EN pin for uSUP stabilisator as output
    digitalWrite(PIN_ON, HIGH);   // Turn on the uSUP power
    int pokusINA = 0; // promena pro pocitani pokusu o pripojeni

    while(! ina219.begin())
    {
      Serial.println("Failed to find INA219 chip, next try");
      delay(500);
      Serial.print(".");
      if(pokusINA > 10)
      {
        Serial.println("Wait 1 min and repeat");
        esp_sleep_enable_timer_wakeup(60000 * 1000);
        esp_deep_sleep_start();
      }
      pokusINA++;
    }

    Serial.println();
    Serial.println("INA219 ready, set calibration on max 16V and 400mA ...");
    ina219.setCalibration_16V_400mA();

  // ============= INA219 ============= //
  
  // ============= SHT40 ============= //
    if (! sht4.begin()) 
      {
        Serial.println("SHT4x not found");
        Serial.println("Check the connection");
        while (1) delay(1);
      }

    sht4.setPrecision(SHT4X_HIGH_PRECISION); // highest resolution
    sht4.setHeater(SHT4X_NO_HEATER); // no heater
  // ============= SHT40 ============= //

}

void loop(void) 
{
  inaF();
  baterkaF();
  dataSHT40();
  wifiF();
  
  Serial.flush();

  // ============= DEEP SLEEP ============= //
    // čas pro funkci deepSleep je milisekundy * 1000
    // hodnota 21 600 000 000 uspí zařízení na 6 hodin: 6 hodin * 60 minut * 60 sekund * 1000 milisekund  
    esp_sleep_enable_timer_wakeup(60000 * 1000); 
    esp_deep_sleep_start();
  // ============= DEEP SLEEP ============= //
}
