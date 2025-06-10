# MonitorAutoPivot

**Landscape orientation** has traditionally been the default for desktop monitors. 
**Portrait orientation**  is better suited for specific use cases, such as:
-   Working with text documents (e.g., Microsoft Word, PDF files).
-   Reading online content (news websites, blogs, reference materials).
-   Graphic design tasks involving portrait photos, posters, book covers, or mobile app interfaces.  
-   Software development, particularly when working with large code files or log outputs.

Nowadays, monitors with built-in rotation mechanisms or those mounted on adjustable brackets are becoming more popular, enabling quick orientation changes. However, switching a monitor's orientation manually can be time-consuming. With MonitorAutoPivot, you can automate changing the orientation of the monitor image with the help of an **external hardware device**. 

# Requirements

This application requires an **external hardware device**. In the current implementation, the system uses:

<img src="https://github.com/user-attachments/assets/af3c22cd-925b-44fe-b43e-c1d7a968b233" alt="Arduino Nano V3" style="width:350px; height:auto;">
<img src="https://github.com/user-attachments/assets/7faa0c74-9058-421b-ae78-936388295981" alt="MPU-6050" style="width:200px; height:auto;">

-   **Arduino Nano V3** + **MPU-6050**
-   USB connection to PC (virtual COM port). *Note - USB cable should support data transfer.*

Application supports Windows 10 and Windows 11. All tests were performed on Windows 10 22H2 build 19045. 

## Features

-  **Automated orientation changing for monitor**
-  **Flexible hardware device mounting** - allows **axis remapping tools** to align accelerometer data with the monitor’s physical movement.
-  **Multi-monitor support** - currently targets one display, while updating layout positions of other monitors in **Extend mode**.
-  **Connection loss alerts**.
-  **User settings saving**.  
-  **Background work**  with auto start up with Windows.

## Usage

- Connect the **MPU-6050 sensor** to your **Arduino Nano V3** using folowwing scheme
- Connect Arduino to PC and Flash the firmware to the device using [Arduino IDE](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started/ide-v2-downloading-and-installing/)
- Mount the device on your monitor.
- Install the Windows application from downloads.
- Launch the application and select Target Monitor.
- Connect to device by selecting its COM-port in app.
- Remap accelerometer axes in app.
- Save settings.
- Enable **Auto rotation** in the app.
- *(Optionally)* Specify layout for correct arrangement of monitors if using multiple monitors. 

## Special Thanks

MonitorAutoPivot was built using frameworks & libraries such as [WinUI 3](https://github.com/microsoft/microsoft-ui-xaml), [nlohmann/json](https://github.com/nlohmann/json), and [WinToast](https://github.com/mohabouje/WinToast).

## Example of work

Picture of device mount:

<img src="https://github.com/user-attachments/assets/b6864b39-c488-4fa1-921c-e384740de80f" alt="device mount" style="width:350px; height:auto;">

Video example of work:

https://github.com/user-attachments/assets/be58321f-edb4-4e1c-9e7e-d8d01325196f




