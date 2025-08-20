# Autonomous Object-Tracking Robotic Car 🚗

## ⚙️ **Summary**  

A dual-mode robotic car built on an **Raspberry Pi 4** that can autonomously track objects or be manually driven via a **PS4 controller**. Implemented in **C++** using **OpenCV** for computer vision, **pigpio** for low-level PWM motor/servo control, and the **Linux joystick API** for input. Optimized for low-latency control (<50 ms) and reliable tracking accuracy (±3 px).  

**Tech Stack:** C++ · OpenCV · pigpio · GStreamer · Linux Joystick API · Raspberry Pi 4 · TB6612FNG Motor Driver · Servo + DC Motors  

**Key Features:** Autonomous HSV-based object tracking · manual override via PS4 controller · low-latency camera + control loop · custom mechanical design  

---

## 🔍 **Explore** 

📄 Project Overview (coming soon)         
🎥 [Educational Demo Video](https://youtu.be/008SZhVjsog)         
🎥 [Fun Demo Video](https://youtube.com/shorts/qCwE8ejM1Z4?feature=share)       

---

## 💡 **Background**  

This project was driven by my interest in the intersection of cars and robotics, exploring how embedded systems and computer vision can enable autonomous vehicle behavior.

---

## 🧠 **A Quick Breakdown**  

**Dual-Mode Operation**  
- *Manual Mode*: PS4 controller input read via `/dev/input/js0` (Linux Joystick API).  
- *Autonomous Mode*: OpenCV processes live camera frames, identifies object position, and adjusts steering/throttle automatically.  

**Computer Vision**  
- Implemented an **HSV-based detection pipeline** (color masking, filtering, contour analysis, circle fitting).  
- Achieved **±3 px accuracy** in varied lighting conditions.  

**Low-Level Motor Control**  
- Used **pigpio** for PWM signals to servo and DC motors.  
- Integrated **TB6612FNG motor driver** for efficient dual-motor actuation.  
- Applied **dynamic PWM scaling + optimized driver logic** for smooth and consistent control.  

**Performance Optimization**  
- Reduced control latency from ~120 ms → <50 ms using:  
  - GStreamer pipeline with leaky-frame queues  
  - Asynchronous camera capture  
  - Non-blocking joystick reads  

---
