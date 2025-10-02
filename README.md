# IoT–ML System for Catfish Pond Water Quality Prediction 🐟💧

This project integrates IoT sensors, a custom PCB, and Machine Learning (Random Forest) to monitor and predict water quality in catfish ponds (biofloc system) in real time.

## 🚀 Features
- **Hardware**: ESP32 microcontroller, sensors (DS18B20 for temperature, pH-4502C for pH, SEN0189 for turbidity), I2C LCD.
- **PCB Design**: Custom schematic & PCB designed with EasyEDA, integrating all sensors and ESP32.
- **Firmware**: Arduino IDE with WiFi + Firebase RTDB + OTA update support.
- **Cloud & Database**: Data transmission to Firebase Realtime Database (RTDB).
- **Machine Learning**: Random Forest model trained on water-quality dataset (98.05% accuracy, F1-score 0.94).
- **Web Dashboard**: Flask backend + HTML/Bootstrap frontend hosted on Firebase, showing real-time graphs, logs, and predictions.

## 🛠 System Architecture
<img width="645" height="353" alt="image" src="https://github.com/user-attachments/assets/d3db7d6e-370f-4da6-8ac0-299eb56b7cba" />

## 📟 Firmware (ESP32)

Developed with Arduino IDE for ESP32. Main workflow:

1. Initialize WiFi, Firebase, sensors (pH, temperature, turbidity), LCD, OTA.  
2. Check WiFi connection → restart if failed.  
3. Read sensors periodically.  
4. Every 3 minutes → average sensor values.  
5. Send data to Firebase RTDB.  
6. Reset variables and repeat.  

## 🔌 Hardware
- Schematic design in EasyEDA  
- Custom PCB (10x9 cm, single-layer)  
- Assembled with ESP32, sensors, and LCD  

<img width="684" height="457" alt="image" src="https://github.com/user-attachments/assets/09818148-410d-4aa8-bd9f-67fd120b3ea3" />
<img width="390" height="352" alt="image" src="https://github.com/user-attachments/assets/73bc5822-e9fb-4f1e-aee4-c28659665cb4" />
<img width="607" height="375" alt="image" src="https://github.com/user-attachments/assets/e30445fd-38dc-4eed-a2b6-244b75d3c394" />


## 📡 Data Pipeline
- Sensors → ESP32 → Firebase RTDB (every 3 minutes)  
- Firebase → ML Model (Random Forest, time-series based)  
- Prediction results stored & visualized on dashboard  

## 🤖 Machine Learning
- Preprocessing: resampling (5s → 3m), interpolation, labeling, SMOTE balancing  
- Model: Random Forest (hyperparameter tuned with Randomized Search)  
- Evaluation: Accuracy 98.05%, F1-score 0.9478  

<img width="632" height="474" alt="image" src="https://github.com/user-attachments/assets/80eebbce-05d3-4465-9a88-fbdbdac46991" />

## 🌐 Web Dashboard
- Dashboard view with real-time water quality & predictions  
- Log table with filter + CSV export  
Link: https://rf-bioflok.web.app/


