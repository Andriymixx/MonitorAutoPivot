#include <Wire.h>

// Flags for state machine
bool remapping = false;
bool sendingData = false;

// Raw data in Axis vars from Accelerometer
float AccX, AccY, AccZ;
// Relative Axis, so device can be placed in any suitable position
float relAccX, relAccY, relAccZ;
// Filtered values of accelerometer
float filteredAccX = 0, filteredAccY = 0, filteredAccZ = 0;
float accAlpha = 0.1;  // filter weight

// for axis remapping
int axisMapX = 0;
int axisMapY = 1;
int axisMapZ = 2;
int axisSignX = 1;
int axisSignY = 1;
int axisSignZ = 1;

int currentOrientation = -1;
int previousOrientation = -1;

// cos(65°) = 0.42, to not update orientation when monitor's pitch is over that value
float orientationThreshold = 0.42;
// Deadzone to not update orientation when device in state betwen two orientations
float deadZoneRatio = 0.1;

// for determening in remaping function
struct RemappingPoint {
  float x, y, z;
};
RemappingPoint remapping90 = { 0 }, remapping0 = { 0 };

// States of remapping process
enum RemappingState {
  REMAP_NONE,
  REMAP_WAIT_90,
  REMAP_WAIT_0,
  REMAP_DONE
};
RemappingState remapState = REMAP_NONE;

// Remaping accelerometer axis
void remapAccAxis() {
  float val90, val0;
  int dominantAxis90 = getDominantAxis(remapping90, val90);
  int dominantAxis0 = getDominantAxis(remapping0, val0);
  // Axes should not be same
  if (dominantAxis90 == dominantAxis0) {
    Serial.println("[REMAPPING_ERR] Error: Same axis for both orientation!");
    remapState = REMAP_NONE;  // leave from remapping state
    remapping = false;
    return;
  }

  // Remaping axis
  axisMapY = dominantAxis90;
  axisSignY = (val90 > 0) ? 1 : -1;
  axisMapZ = dominantAxis0;
  axisSignZ = (val0 > 0) ? 1 : -1;

  // last axis is X
  int allAxes[3] = { 0, 1, 2 };
  for (int i = 0; i < 3; i++) {
    if (i != axisMapY && i != axisMapZ) {
      axisMapX = i;
      axisSignX = 1;
      break;
    }
  }
  remapState = REMAP_DONE;  // remapping succeeded
}

// get dominant axis to help determing axis remaping
int getDominantAxis(const RemappingPoint& point, float& valueOut) {
  float vals[3] = { point.x, point.y, point.z };
  int maxIndex = 0;
  float maxVal = fabs(vals[0]);

  for (int i = 1; i < 3; i++) {
    if (fabs(vals[i]) > maxVal) {
      maxVal = fabs(vals[i]);
      maxIndex = i;
    }
  }

  valueOut = vals[maxIndex];
  return maxIndex;
}

// Simplified realization of complementary filter
void filterAccelerometer() {
  filteredAccX = (1 - accAlpha) * filteredAccX + accAlpha * relAccX;
  filteredAccY = (1 - accAlpha) * filteredAccY + accAlpha * relAccY;
  filteredAccZ = (1 - accAlpha) * filteredAccZ + accAlpha * relAccZ;
  // filteredAccX =  relAccX;
  // filteredAccY = relAccY;
  // filteredAccZ = relAccZ;
}

// For calculating orientation
struct Orientation {
  float x, y, z;
  int angle;
};
// In an ideal situation the axis is equal to 1 or -1, so with that can be determined orientation
Orientation orientations[] = {
  { 0, 0, 1, 0 },
  { 0, 0, -1, 180 },
  { 0, 1, 0, 90 },
  { 0, -1, 0, 270 },
};

void acc_signals(void) {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);  //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0x68, 6);                         //Request Accel Registers (3B - 40)
  int16_t AccXLSB = Wire.read() << 8 | Wire.read();  //Store first two bytes into accelX
  int16_t AccYLSB = Wire.read() << 8 | Wire.read();  //Store middle two bytes into accelY
  int16_t AccZLSB = Wire.read() << 8 | Wire.read();  //Store last two bytes into accelZ
  // Convert LSB in g and apply correction values
  AccX = (float)AccXLSB / 16384 - 0.06;
  AccY = (float)AccYLSB / 16384 - 0.02;
  AccZ = (float)AccZLSB / 16384 - 0.02;
  // float tempAcc[3] = { AccZ, AccY, AccX };
  // Saving relative axis according to remap
  float tempAcc[3] = { AccX, AccY, AccZ };
  relAccX = tempAcc[axisMapX] * axisSignX;
  relAccY = tempAcc[axisMapY] * axisSignY;
  relAccZ = tempAcc[axisMapZ] * axisSignZ;
}

void setup() {
  Serial.begin(115200);
  // Initialize I2C
  Wire.setClock(400000);
  Wire.begin();
  delay(250);
  Wire.beginTransmission(0x68);  // I2C address of the MPU
  Wire.write(0x6B);              //Accessing the register 6B - Power Management
  Wire.write(0x00);              //Setting SLEEP register to 0
  Wire.endTransmission();
  // Set sensitivity of accelerometer
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);             // Accessing the register 1C - ACCEL_CONFIG
  Wire.write(0x00);             // set sensitivity of ±8g (resolution: 4096 LSB/g)
  Wire.endTransmission();
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  // Sending to pc that Arduino is ready
  Serial.println("[READY]");
}

void loop() {
  if (Serial.available()) {
    // Reading command from pc
    String command = Serial.readStringUntil('\n');
    command.trim();
    if (command == "REMAP_AXIS") {
      remapping = true;  // Set flag to remapping state
      remapState = REMAP_WAIT_90;
    } else if (command == "OK" && remapping) {
      // Firstly record 90 (portrait) orientation
      if (remapState == REMAP_WAIT_90) {
        remapping90 = { filteredAccX, filteredAccY, filteredAccZ };
        Serial.println("Portrait orientation recorded");
        remapState = REMAP_WAIT_0;  // change state of remapping to next ste
      } else if (remapState == REMAP_WAIT_0) {
        // Secondly record 0 (landscape) orientation
        remapping0 = { filteredAccX, filteredAccY, filteredAccZ };
        Serial.println("Landscape orientation recorded");
        remapAccAxis();
      }
    } else if (command == "STOP" && remapping) {
      // if User cancelled remapping
      Serial.println("[REMAPPING_ERR] Cancelled");
      remapState = REMAP_NONE;
      remapping = false;

    } else if (command.startsWith("AUTO_REMAP")) {
      // Loading remap of axis from config
      command.remove(0, 11);  // remove "AUTO_REMAP/"
      // For 6 variables
      int parts[6] = { 0 };
      int index = 0;
      int lastSlash = -1;

      // split by "/"
      for (int i = 0; i < command.length(); i++) {
        if (command[i] == '/') {
          String part = command.substring(lastSlash + 1, i);
          parts[index++] = part.toInt();
          lastSlash = i;
          if (index >= 6) break;
        }
      }
      // Add last index after '/'
      if (index < 6) {
        parts[index] = command.substring(lastSlash + 1).toInt();
      }
      // Assign remap
      axisMapX = parts[0];
      axisMapY = parts[1];
      axisMapZ = parts[2];
      axisSignX = parts[3];
      axisSignY = parts[4];
      axisSignZ = parts[5];
      Serial.println("[REMAP_LOADED]");
    } else if (command == "SEND_DATA") {
      sendingData = true;  // Start sending orientation to pc
    } else if (command == "STOP" && sendingData) {
      // Stop sending to pc and clear flags about orientation
      sendingData = false;
      currentOrientation = -1;
      previousOrientation = -1;
      Serial.println("[SENDING_STOPPED]");
    }
  }

  // --- state machine regular functions ---
  if (remapping) {
    switch (remapState) {
      case REMAP_WAIT_90:
        acc_signals();
        filterAccelerometer();
        Serial.print("[DATA] Acc X [g]= ");
        Serial.print(filteredAccX);
        Serial.print(" Acc Y [g]= ");
        Serial.print(filteredAccY);
        Serial.print(" Acc Z [g]= ");
        Serial.println(filteredAccZ);
        delay(50);
        Serial.println("[Instruction] Turn device to portrait orientation");
        break;
      case REMAP_WAIT_0:
        acc_signals();
        filterAccelerometer();
        Serial.print("[DATA] Acc X [g]= ");
        Serial.print(filteredAccX);
        Serial.print(" Acc Y [g]= ");
        Serial.print(filteredAccY);
        Serial.print(" Acc Z [g]= ");
        Serial.println(filteredAccZ);
        Serial.println("[Instruction] Turn device to landscape orientation");
        delay(50);
        break;
      case REMAP_DONE:
        Serial.print("[REMAPPING_DONE] Remapping Done");
        // Send values to pc to save in config
        Serial.print(" /");
        Serial.print(axisMapX);
        Serial.print("/");
        Serial.print(axisMapY);
        Serial.print("/");
        Serial.print(axisMapZ);
        Serial.print("/");
        Serial.print(axisSignX);
        Serial.print("/");
        Serial.print(axisSignY);
        Serial.print("/");
        Serial.println(axisSignZ);
        remapping = false;  // leave from remapping state
        remapState = REMAP_NONE;
        break;
      default:
        break;
    }

  } else if (sendingData) {
    // Detecting current orientation and sending new orientation to pc
    acc_signals();
    filterAccelerometer();
    // variable for comparison with a reference value for the corresponding orientation
    float bestDot = -1;
    // second one for comparising both of them
    float secondBestDot = -1;
    // best matching orientation variable
    int bestAngle;

    // Check which dot have best match for every orientation
    for (int i = 0; i < 4; i++) {
      // Calculating the value dot
      float dot = filteredAccX * orientations[i].x + filteredAccY * orientations[i].y + filteredAccZ * orientations[i].z;

      if (dot > bestDot) {
        // if new dot is new best
        secondBestDot = bestDot;
        bestDot = dot;
        // save best matching orientation
        bestAngle = orientations[i].angle;
      } else if (dot > secondBestDot) {
        // if new dot better than second secondBestDot
        secondBestDot = dot;
      }
    }
    // Update current orientation only if monitor's pitch less than specified value
    if (bestDot > orientationThreshold) {
      // Deadzone if the difference between the two best dots is less than 10% from best dot
      if ((bestDot - secondBestDot) < (deadZoneRatio * bestDot)) {
        currentOrientation = previousOrientation;
      } else {
        // otherwise update new orientation
        currentOrientation = bestAngle;
      }
    }
    // Check if orientation has changed and send it to pc
    if (currentOrientation != previousOrientation) {
      previousOrientation = currentOrientation;
      Serial.print("[DATA] ");
      Serial.println(currentOrientation);
    }
    // Serial.print("Orientation: ");
    // Serial.print(currentOrientation);
    // Serial.print("AccX:");
    // Serial.print(filteredAccX);
    // Serial.print(',');
    // Serial.print("AccY:");
    // Serial.print(filteredAccY);
    // Serial.print(',');
    // Serial.print("AccZ:");
    // Serial.print(filteredAccZ);
    // Serial.println();
  }
  delay(50);
}
