// ===========================================================
// Quantum-Inspired Green Hydrogen Forecasting (ESP32 + RGB LED)
// Full Version with 3-Bar Serial Graph + HYBRID SENSOR MODE
// With Quantum Memory Weight (Stabilization) + OLED Bar Graph + Live Line Graph
// ===========================================================

#include <Wire.h>                      // Include Wire library for I2C communication
#include <Adafruit_GFX.h>              // Include Adafruit GFX library for graphics
#include <Adafruit_SSD1306.h>          // Include Adafruit SSD1306 library for OLED display

// --- SCREEN CONFIG ---
#define SCREEN_WIDTH 128               // Define the screen width for the OLED display
#define SCREEN_HEIGHT 64               // Define the screen height for the OLED display
#define GRAPH_WIDTH 128                // Define the width of the graph
#define GRAPH_HEIGHT 50                // Define the height of the graph

// --- TWO DISPLAY BUSSES ---
TwoWire I2Cone = TwoWire(0);           // Define the first I2C bus (I2Cone)
TwoWire I2Ctwo = TwoWire(1);           // Define the second I2C bus (I2Ctwo)

// --- DISPLAY DEFINITIONS ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Cone, -1);    // Initialize the first OLED display
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &I2Ctwo, -1);   // Initialize the second OLED display

// --- Pin Definitions ---
const int LDR_AO_PIN = 35;            // Define the pin for the Light Dependent Resistor (LDR)
const int RGB_RED_PIN = 25;           // Define the pin for the RGB red LED
const int RGB_GREEN_PIN = 26;         // Define the pin for the RGB green LED
const int RGB_BLUE_PIN = 27;          // Define the pin for the RGB blue LED

// --- Variables ---
int ldrValue = 0;                     // Variable to store LDR value
int potValue = 0;                     // Variable to store potentiometer value
int readingCount = 0;                 // Variable to count readings

float SIM_STRENGTH_SOLAR = 0.30;      // Simulated solar strength factor
String lastForecast = "remain";      // Last forecasted value
float memoryStrength = 0.25;          // Memory strength for forecasting stabilization

// OLED2 graph variables
int graphX = 0;                       // X position for the graph
float previousValue = -1;             // Previous value for drawing the graph
int markerCounter = 0;                // Counter for graph marker

// Previous hydrogen production tracking
float previousHydrogenProduction = -1;    // Previous hydrogen production
float previousHydrogenPercentage = -1;    // Previous hydrogen production percentage

// Function prototypes
void drawHydrogenGraphOnOLED2(float hydrogenPercent);  // Function to draw the hydrogen graph
void printBarGraph3(float solar, float water, float energyPotential, String forecast); // Function to print the bar graph
void drawBarGraphOLED(float solar, float water, float energyPotential);  // Function to draw the bar graph on OLED
void fadeToColor(int r, int g, int b, int duration);   // Function to fade RGB color

// ===========================================================
void setup() {

  Serial.begin(115200);                // Initialize the serial communication with baud rate 115200

  // --- OLED 1 SETUP ---
  I2Cone.begin(21, 22, 400000);        // Begin communication for OLED 1 (I2C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Initialize OLED 1 with the specified I2C address
    Serial.println(F("OLED 1 FAILED. Check wiring."));  // Display an error message if OLED 1 fails
    for(;;);  // Infinite loop to stop execution if OLED 1 fails
  }

  // --- OLED 2 SETUP ---
  I2Ctwo.begin(19, 23, 400000);        // Begin communication for OLED 2 (I2C)
  if (!display2.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Initialize OLED 2 with the specified I2C address
    Serial.println(F("OLED 2 FAILED. Check wiring.")); // Display an error message if OLED 2 fails
    for(;;);  // Infinite loop to stop execution if OLED 2 fails
  }

  delay(500);                          // Delay to allow OLEDs to initialize

  display.clearDisplay();               // Clear the OLED 1 display
  display.setTextSize(1);               // Set text size to 1 on OLED 1
  display.setTextColor(WHITE);          // Set text color to white on OLED 1
  display.setCursor(0, 10);             // Set cursor position on OLED 1
  display.println("Booting System..."); // Display "Booting System..." on OLED 1
  display.display();                    // Update OLED 1 to show the text

  display2.clearDisplay();              // Clear the OLED 2 display
  display2.setTextSize(1);              // Set text size to 1 on OLED 2
  display2.setTextColor(WHITE);         // Set text color to white on OLED 2
  display2.setCursor(0, 10);            // Set cursor position on OLED 2
  display2.println("Booting System...");// Display "Booting System..." on OLED 2
  display2.display();                   // Update OLED 2 to show the text

  delay(1000);                         // Delay for 1 second

  Serial.println();
  Serial.println("Quantum-Inspired Green Hydrogen Forecasting"); // Print system name
  Serial.println("------------------------------------------------");
  Serial.println("System ready.\n");

  pinMode(RGB_RED_PIN, OUTPUT);        // Set the RGB red pin as output
  pinMode(RGB_GREEN_PIN, OUTPUT);      // Set the RGB green pin as output
  pinMode(RGB_BLUE_PIN, OUTPUT);       // Set the RGB blue pin as output

  digitalWrite(RGB_RED_PIN, HIGH);    // Set the RGB red pin high (turn off red)
  digitalWrite(RGB_GREEN_PIN, HIGH);  // Set the RGB green pin high (turn off green)
  digitalWrite(RGB_BLUE_PIN, HIGH);   // Set the RGB blue pin high (turn off blue)

  randomSeed(analogRead(36));         // Initialize random seed using analogRead from pin 36

  Serial.println("CSV:solar,water,forecast");  // Print CSV header for solar, water, and forecast
}

// ===========================================================
void loop() {

  int realLDR = analogRead(LDR_AO_PIN);   // Read the LDR value from the analog pin

  static float t2 = 0;                    // Define a static variable for simulating LDR values
  static float t = 0;                     // Define a static variable for simulating potentiometer values

  int simLDR = (sin(t2) * 0.5 + 0.5) * 4095;  // Simulate LDR value based on sine wave
  int simPOT = (sin(t) * 0.5 + 0.5) * 4095;  // Simulate potentiometer value based on sine wave

  t2 += 0.008;                           // Increment t2 for the next simulation cycle
  t += 0.01;                             // Increment t for the next simulation cycle

  ldrValue = (1.0 - SIM_STRENGTH_SOLAR) * realLDR + SIM_STRENGTH_SOLAR * simLDR; // Combine real and simulated LDR values
  potValue = simPOT;                     // Assign simulated potentiometer value

  float invertedLdrValue = 4095.0 - ldrValue;  // Invert LDR value for further calculations

  float solar = invertedLdrValue / 4095.0;     // Normalize solar value between 0 and 1
  float water = potValue / 4095.0;             // Normalize water value between 0 and 1
  float energyPotential = (0.6 * solar + 0.4 * water); // Calculate energy potential from solar and water values

  float p_increase = energyPotential;          // Probability for increase
  float p_same = (1 - fabs(solar - water)) * 0.3; // Probability for remain
  float p_decrease = 1.0 - (p_increase + p_same);  // Probability for decrease

  if (p_decrease < 0) p_decrease = 0;         // Ensure probability for decrease is not negative

  float total = p_increase + p_same + p_decrease; // Total probability
  p_increase /= total;                        // Normalize increase probability
  p_same /= total;                            // Normalize remain probability
  p_decrease /= total;                        // Normalize decrease probability

  if (lastForecast == "increase") p_increase += memoryStrength;  // Adjust increase probability based on last forecast
  else if (lastForecast == "remain") p_same += memoryStrength;   // Adjust remain probability based on last forecast
  else if (lastForecast == "decrease") p_decrease += memoryStrength; // Adjust decrease probability based on last forecast

  total = p_increase + p_same + p_decrease;   // Total probability again after adjustment
  p_increase /= total;                        // Renormalize increase probability
  p_same /= total;                            // Renormalize remain probability
  p_decrease /= total;                        // Renormalize decrease probability

  // --- QUANTUM NOISE  ---
  // Adding quantum-inspired randomness to simulate quantum behavior
  float quantumNoise = (random(-300, 300) / 1000.0); // Generate random quantum noise

  p_increase += quantumNoise * 0.4;             // Apply noise to increase probability
  p_same     += quantumNoise * -0.2;             // Apply noise to remain probability (inverse coupling)
  p_decrease += quantumNoise * 0.4;             // Apply noise to decrease probability

  // Prevent negatives
  if (p_increase < 0) p_increase = 0;           // Ensure no negative probability for increase
  if (p_same     < 0) p_same     = 0;           // Ensure no negative probability for remain
  if (p_decrease < 0) p_decrease = 0;           // Ensure no negative probability for decrease

  // Renormalize again
  total = p_increase + p_same + p_decrease;     // Total probability again after noise application
  p_increase /= total;                          // Renormalize increase probability
  p_same     /= total;                          // Renormalize remain probability
  p_decrease /= total;                          // Renormalize decrease probability

  float r = random(0, 1000) / 1000.0;           // Generate random number between 0 and 1

  String forecast;                              // Variable to store the forecast
  int targetR = 0, targetG = 0, targetB = 0;     // RGB values for color feedback

  // Determine forecast based on hydrogen production change
  float currentHydrogenPercentage = energyPotential * 100; // Calculate the current hydrogen production percentage

  if (previousHydrogenPercentage < 0) {  // First reading, default to remain
    forecast = "remain";
    targetB = 255;                       // Set blue color for remain forecast
  } else {
    if (currentHydrogenPercentage > previousHydrogenPercentage) { // If production is increasing
      forecast = "increase";
      targetG = 255;                     // Set green color for increase forecast
    } else if (currentHydrogenPercentage < previousHydrogenPercentage) { // If production is decreasing
      forecast = "decrease";
      targetR = 255;                     // Set red color for decrease forecast
    } else {                             // If no change
      forecast = "remain";
      targetB = 255;                     // Set blue color for remain forecast
    }
  }

  // Update previous hydrogen percentage
  previousHydrogenPercentage = currentHydrogenPercentage;

  lastForecast = forecast;  // Update last forecast value

  // Print solar, water, and forecast values to the serial monitor
  Serial.print("CSV:");
  Serial.print(solar, 3);
  Serial.print(",");
  Serial.print(water, 3);
  Serial.print(",");
  Serial.println(forecast);

  readingCount++;          // Increment reading count
  if (readingCount >= 10) {  // If 10 readings are reached
    Serial.print("solar:");
    Serial.println(solar * 100);
    Serial.print("water:");
    Serial.println(water * 100);
    readingCount = 0;       // Reset reading count
  }

  Serial.println("------------------------------------------------");
  printBarGraph3(solar, water, energyPotential, forecast); // Print the bar graph to the serial monitor
  Serial.println("------------------------------------------------\n");

  // --- OLED1 and OLED2 Bar Graphs ---
  auto updateOLED = [&](Adafruit_SSD1306 &scr) {  // Update OLED displays with forecast values
    scr.clearDisplay();                         // Clear the display
    scr.setTextSize(1);                         // Set text size
    scr.setTextColor(WHITE);                    // Set text color to white

    scr.setCursor(0, 0);                       // Set cursor to top left
    scr.println("H2 FORECAST SYSTEM");         // Display "H2 FORECAST SYSTEM" on OLED
    scr.drawLine(0, 9, 128, 9, WHITE);         // Draw line for section separation

    scr.setCursor(0, 12);                      // Set cursor for solar data
    scr.print("Solar: ");
    scr.print(solar * 100, 0);
    scr.println("%");

    scr.setCursor(0, 22);                      // Set cursor for water data
    scr.print("Water: ");
    scr.print(water * 100, 0);
    scr.println("%");

    drawBarGraphOLED(solar, water, energyPotential); // Draw the bar graph on OLED

    scr.setCursor(0, 56);                      // Set cursor for forecast data
    scr.print("FCST: ");
    if (forecast == "increase") scr.print("INCREASE (+)");
    else if (forecast == "remain") scr.print("STABLE (=)");
    else scr.print("DECREASE (-)");

    scr.display();                             // Update the display
  };

  updateOLED(display); // OLED1 bar + forecast

  // --- OLED2 live line graph ---
  drawHydrogenGraphOnOLED2(energyPotential * 100);  // Draw live hydrogen production graph on OLED2

  fadeToColor(targetR, targetG, targetB, 100);  // Fade RGB LEDs to the target color based on forecast

  delay(100); // Delay to slow down loop execution
}

// ===========================================================
void printBarGraph3(float solar, float water, float energyPotential, String forecast) {

  int solarBar = solar * 50;             // Calculate the solar bar length based on solar percentage
  int waterBar = water * 50;             // Calculate the water bar length based on water percentage
  int hydrogenBar = energyPotential * 50; // Calculate the hydrogen bar length based on energy potential

  Serial.println();
  Serial.println("Graph (each '#' about 2 percent)");

  // Print the solar bar graph to the serial monitor
  Serial.print("Solar : [");
  for (int i = 0; i < solarBar; i++) Serial.print("#");
  for (int i = solarBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(solar * 100, 1);
  Serial.println("%");

  // Print the water bar graph to the serial monitor
  Serial.print("Water : [");
  for (int i = 0; i < waterBar; i++) Serial.print("#");
  for (int i = waterBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(water * 100, 1);
  Serial.println("%");

  // Print the hydrogen bar graph to the serial monitor
  Serial.print("H2 : [");
  for (int i = 0; i < hydrogenBar; i++) Serial.print("#");
  for (int i = hydrogenBar; i < 50; i++) Serial.print(" ");
  Serial.print("] ");
  Serial.print(energyPotential * 100, 1);
  Serial.print("% -> ");
  Serial.println(forecast);
}

// ===========================================================
void drawBarGraphOLED(float solar, float water, float energyPotential) {

  const int maxChars = 18;               // Maximum number of characters for the bar graph
  int solarChars = (int)(solar * maxChars);  // Calculate solar bar characters
  int waterChars = (int)(water * maxChars);  // Calculate water bar characters
  int h2Chars = (int)(energyPotential * maxChars);  // Calculate hydrogen bar characters

  if (solarChars > maxChars) solarChars = maxChars;  // Ensure solarChars doesn't exceed maxChars
  if (waterChars > maxChars) waterChars = maxChars;  // Ensure waterChars doesn't exceed maxChars
  if (h2Chars > maxChars) h2Chars = maxChars;        // Ensure h2Chars doesn't exceed maxChars

  int startY = 32;                         // Starting Y position for the bar graph
  int lineH = 7;                           // Line height for the bars

  display.setTextSize(1);                  // Set text size for the OLED display
  display.setTextColor(WHITE);             // Set text color to white

  // Draw solar bar graph
  display.setCursor(0, startY);
  display.print("S:");
  for (int i = 0; i < maxChars; i++) display.print(i < solarChars ? "#" : " ");

  // Draw water bar graph
  display.setCursor(0, startY + lineH);
  display.print("W:");
  for (int i = 0; i < maxChars; i++) display.print(i < waterChars ? "#" : " ");

  // Draw hydrogen bar graph
  display.setCursor(0, startY + 2 * lineH);
  display.print("H:");
  for (int i = 0; i < maxChars; i++) display.print(i < h2Chars ? "#" : " ");
}

// ===========================================================
void fadeToColor(int r, int g, int b, int duration) {

  static int currR = 0, currG = 0, currB = 0; // Initialize current RGB values

  int steps = 50;                          // Number of steps for fading
   int stepDelay = duration / steps;         // Calculate the delay per step for smooth fading

  for (int i = 0; i <= steps; i++) {

    // Gradually interpolate between the current color and the target color
    int newR = currR + (r - currR) * i / steps;
    int newG = currG + (g - currG) * i / steps;
    int newB = currB + (b - currB) * i / steps;

    // Adjust RGB LED colors based on the fading values
    analogWrite(RGB_RED_PIN, 255 - newR);   // Set red LED based on the new value
    analogWrite(RGB_GREEN_PIN, 255 - newG); // Set green LED based on the new value
    analogWrite(RGB_BLUE_PIN, 255 - newB);  // Set blue LED based on the new value

    delay(stepDelay);                       // Delay to create the fade effect
  }

  // Update the current RGB values to the target values after fading
  currR = r;
  currG = g;
  currB = b;
}

// ===========================================================
// OLED2 LEFT-TO-RIGHT LIVE LINE GRAPH
// ===========================================================
void drawHydrogenGraphOnOLED2(float hydrogenPercent) {

  // Map the hydrogen percentage to the OLED's vertical axis (Y-axis)
  int y = map(hydrogenPercent, 0, 100, GRAPH_HEIGHT, 0); // invert Y axis for correct graph orientation

  // Only clear at the start of the graph
  if (graphX == 0) {
    display2.clearDisplay();                             // Clear the display
    display2.drawLine(0, 0, 0, GRAPH_HEIGHT, WHITE);      // Draw the Y-axis line
    display2.drawLine(0, GRAPH_HEIGHT, GRAPH_WIDTH, GRAPH_HEIGHT, WHITE); // Draw the X-axis line
    display2.setTextSize(1);                             // Set text size for the graph title
    display2.setCursor(10, 0);                           // Set cursor for graph label
    display2.print("H2 Production (%)");                  // Print graph label on the OLED display
    previousValue = y;                                   // Set the initial value for the first point on the graph
  }

  // Draw connecting line from the previous value to the new value
  if (previousValue >= 0) {
    display2.drawLine(graphX - 1, previousValue, graphX, y, WHITE); // Draw line between previous and current points
  }

  // Draw marker every few loops to highlight specific points
  markerCounter++;                                         // Increment the marker counter
  if (markerCounter >= 2) {                                 // Draw a marker after every 2 iterations
    display2.drawPixel(graphX, y, WHITE);                   // Draw a pixel marker
    markerCounter = 0;                                      // Reset marker counter
  }

  previousValue = y;                                       // Update the previous value for the next point
  graphX++;                                                // Move to the next graph position (X-axis)

  // Reset the graph when the X position reaches the end
  if (graphX >= GRAPH_WIDTH) {
    graphX = 0;                                            // Reset X position to 0 (start of the graph)
    previousValue = -1;                                    // Reset previous value for the next graph cycle
  }

  // **Do not clear the display here!** Only update the graph with new points
  display2.display();                                       // Update OLED2 with the new graph data
}

