//all the libraries we need
#include <opencv2/opencv.hpp>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <pigpio.h>
#include <csignal>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

using namespace std;
using namespace cv;


// Pins on the raspberry pi 4
// Motor pins
#define PWMA      18
#define AIN1      23
#define AIN2      24
// Servo pin 
#define SERVO_PIN 25

// Joystick mappings
const int throttleAxis     = 1;  // left stick vertical
const int steeringAxis     = 3;  // right stick horizontal
const int modeSwitchButton = 0;  // “X” button

// Servo geometry
const int SERVO_CENTER = 1850;  
const int SERVO_RANGE  = 500;    

// Autonomous drive settings
const int AUTO_THROTTLE_HIGH    = 240;    // out of 255 to avoid burning motor out
const int AUTO_THROTTLE_LOW     = 0;
const float RADIUS_THRESHOLD_PX = 100.0f; // stop when radius exceeds this

volatile bool running = true;
void handleSignal(int) { running = false; }

int main() {
    // catch Ctrl-C to exit terminal wihtout crashing the program and causing memory failures
    signal(SIGINT,  handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGHUP,  handleSignal);

    // open joystick API
    int js = open("/dev/input/js0", O_RDONLY | O_NONBLOCK);
    if (js < 0) { perror("open /dev/input/js0"); return 1; }

    // init pigpio
    if (gpioInitialise() < 0) { cerr<<"pigpio init failed\n"; return 1; }
    gpioSetMode(PWMA,      PI_OUTPUT);
    gpioSetMode(AIN1,      PI_OUTPUT);
    gpioSetMode(AIN2,      PI_OUTPUT);
    gpioSetMode(SERVO_PIN, PI_OUTPUT);
    gpioServo(SERVO_PIN,   SERVO_CENTER);

    cout<<" Ready. Press X to toggle MANUAL/AUTO.\n";

    // camera pipeline: 320×240, drop old frames for saving memory and ensuring smoothness
    string pipeline =
      "libcamerasrc ! video/x-raw,width=320,height=240,format=RGB ! "
      "queue max-size-buffers=1 leaky=downstream ! "
      "videoconvert ! video/x-raw,format=BGR ! appsink drop=true sync=false";
    VideoCapture cap(pipeline, CAP_GSTREAMER);
    if (!cap.isOpened()) {
        cerr<<"Cannot open camera\n";
        return 1;
    }

    bool autoMode   = false;
    bool autoActive = true;  // our latch flag
    struct js_event e;
    const int deadzone = 4000;
    Mat frame, hsv, mask;

    while (running) {
        int frameW = 320;
        float bestR = 0, bestCX = -1;

        // autonomous mode operation
        if (autoMode && autoActive && cap.grab()) {
            cap.retrieve(frame);
            if (!frame.empty()) {
                frameW = frame.cols;
                cvtColor(frame, hsv, COLOR_BGR2HSV);
                inRange(hsv, Scalar(25,100,100), Scalar(45,255,255), mask);
                erode(mask, mask, Mat(), Point(-1,-1), 1);
                dilate(mask, mask, Mat(), Point(-1,-1), 1);

                vector<vector<Point>> contours;
                findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
                for (auto &c : contours) {
                    Point2f c0; float r0;
                    minEnclosingCircle(c, c0, r0);
                    if (r0 > bestR && r0 > 5) {
                        bestR  = r0;
                        bestCX = c0.x;
                    }
                }

                if (bestR > 0) {
                    // debug print radius
                    cout<<" radius="<<bestR<<" px     \r";
                    cout.flush();

                    // steering
                    float norm = (bestCX - frameW/2.0f) / (frameW/2.0f);
                    int pulse = SERVO_CENTER - int(norm * SERVO_RANGE);
                    gpioServo(SERVO_PIN,
                        clamp(pulse,
                              SERVO_CENTER - SERVO_RANGE,
                              SERVO_CENTER + SERVO_RANGE)
                    );

                    // throttle
                    if (bestR < RADIUS_THRESHOLD_PX) {
                        gpioWrite(AIN1, 0);
                        gpioWrite(AIN2, 1);
                        gpioPWM(PWMA, AUTO_THROTTLE_HIGH);
                    } else {
                        // stop and latch off
                        gpioPWM(PWMA, AUTO_THROTTLE_LOW);
                        autoActive = false;
                        cout<<"\n AUTO latched off at radius ≥ "<<RADIUS_THRESHOLD_PX<<"\n";
                    }
                }
            }
        }

        // manual mode operation
        while (true) {
            ssize_t r = read(js, &e, sizeof(e));
            if (r != sizeof(e)) break;
            // X button toggles mode
            if ((e.type & JS_EVENT_BUTTON)
             && e.number == modeSwitchButton
             && e.value  == 1) {
                autoMode   = !autoMode;
                autoActive = autoMode;  // reset latch when re-entering AUTO
                cout<<(autoMode?" AUTO\n":" MANUAL\n");
                gpioServo(SERVO_PIN, SERVO_CENTER);
                gpioPWM(PWMA, 0);
            }
            // joystick axes only in MANUAL
            if (!autoMode && (e.type & JS_EVENT_AXIS)) {
                float n = (abs(e.value)>deadzone
                         ? e.value/32767.0f
                         : 0.0f);
                if (e.number == throttleAxis) {
                    int d = int(abs(n)*255);
                    if      (n > 0) { gpioWrite(AIN1,1); gpioWrite(AIN2,0); gpioPWM(PWMA,d); }
                    else if (n < 0) { gpioWrite(AIN1,0); gpioWrite(AIN2,1); gpioPWM(PWMA,d); }
                    else            { gpioPWM(PWMA,0); gpioWrite(AIN1,0); gpioWrite(AIN2,0); }
                }
                else if (e.number == steeringAxis) {
                    n = -n;
                    int p = int(SERVO_CENTER + n*SERVO_RANGE);
                    gpioServo(SERVO_PIN,
                        clamp(p,
                              SERVO_CENTER - SERVO_RANGE,
                              SERVO_CENTER + SERVO_RANGE)
                    );
                }
            }
        }

        gpioDelay(500);  // 0.5 ms
    }

    // cleanup routine
    gpioPWM(PWMA,0);
    gpioWrite(AIN1,0);
    gpioWrite(AIN2,0);
    gpioServo(SERVO_PIN, SERVO_CENTER);
    gpioTerminate();
    close(js);
    return 0;
}
