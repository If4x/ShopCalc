/*
This is a simple web-based cash register system for the ESP32.
Developed by Imanuel Fehse (2025).

It's intended to serve as a basic shop calculator for school events or similar to optimize the process of sellig products timewise.
*/


// Libraries for ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

const char* ssid = "Kasse";         // SSID of th WIFI
const char* password = "BitteGeld"; // Password for WIFI

// Port 80 (Kassenseite) und Port 8080 (Konfigurationsseite)
// Standard IP for webserver is 192.168.4.1
WebServer server(80);        // product page
WebServer configServer(8080); // config page

#define MAX_PRODUCTS 20
#define EEPROM_SIZE 4096
#define LED_PIN 2  // GPIO der Onboard-LED (meist GPIO 2)
#define SALES_EEPROM_ADDR 600
#define EEPROM_PRODUCTS_START 0
#define EEPROM_SALES_START 1000

unsigned long previousMillis = 0;
const long interval = 900; // blinking interval
bool ledOn = false; // state of status led

struct Product {
  char name[30]; // product name max 30 chars due to EEPROM size limitations
  float price; // two decimal places
  bool hasDeposit; // true if product has deposit
  int count; // number of products in cart
  int sold; // number of products sold (for sales overview)
};

Product products[MAX_PRODUCTS]; // Array for products
int productCount = 0;

int totalSold[MAX_PRODUCTS]; // cumulative number sold per product


// if EEPROM is empty, default products are loaded
Product defaultProducts[] = {
  {"Brezel", 2.00, false, 0},
  {"Fanta", 2.50, true, 0},
  {"Cola", 2.50, true, 0},
  {"Spezi", 3.00, true, 0},
  {"Apfelschorle", 3.00, true, 0},
  {"Ensinger Medium", 2.00, true, 0},
  {"Ensinger Still", 2.00, true, 0},
  {"Bier", 3.00, true, 0},
  {"Sekt", 3.00, true, 0}
};

const int defaultProductCount = sizeof(defaultProducts) / sizeof(defaultProducts[0]);

void saveSalesToEEPROM() {
  for (int i = 0; i < productCount; i++) {
    EEPROM.writeInt(EEPROM_SALES_START + i * sizeof(int), totalSold[i]);
  }
  EEPROM.commit();
}

void loadSalesFromEEPROM() {
  for (int i = 0; i < productCount; i++) {
    totalSold[i] = EEPROM.readInt(EEPROM_SALES_START + i * sizeof(int));
  }
}


// EEPROM management
// save to EEPROM
void saveProductsToEEPROM() {
  EEPROM.write(0, productCount);
  int addr = 1;
  for (int i = 0; i < productCount; i++) {
    EEPROM.put(addr, products[i]);
    addr += sizeof(Product);
  }
  EEPROM.commit();
}

// load/read from EEPROM
void loadProductsFromEEPROM() {
  productCount = EEPROM.read(0);
  if (productCount <= 0 || productCount > MAX_PRODUCTS) {
    productCount = 0;
    return;
  }
  int addr = 1;
  for (int i = 0; i < productCount; i++) {
    EEPROM.get(addr, products[i]);
    products[i].count = 0; // Zähler zurücksetzen
    addr += sizeof(Product);
  }
}

// cacluate total price of all products in cart
float calculateTotal() {
  float total = 0;
  for (int i = 0; i < productCount; i++) {
    total += products[i].count * products[i].price;
    if (products[i].hasDeposit) total += products[i].count * 1.0;
  }
  return total;
}

// calculate total deposit of all products in cart (is gonna be shown as already included in total price)
float calculateDeposit() {
  float deposit = 0;
  for (int i = 0; i < productCount; i++) {
    if (products[i].hasDeposit) deposit += products[i].count * 1.0;
  }
  return deposit;
}


void handleSalesOverview() {
  loadSalesFromEEPROM(); // load sales from EEPROM
  
  // Start the HTML content
  String html = "<h1>Verkäufe</h1>";
  
  // Create a table for the sales
  html += "<table border='1'><tr><th>Produkt</th><th>Anzahl</th></tr>";
  
  // Loop through the products and add them to the table
  for (int i = 0; i < productCount; i++) {
    html += "<tr><td>" + String(products[i].name) + "</td><td>" + String(totalSold[i]) + "</td></tr>";
  }
  Serial.print("productCount: ");
  Serial.println(productCount);
  for (int i = 0; i < productCount; i++) {
    Serial.print("Product ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(products[i].name);
    Serial.print(" - verkauft: ");
    Serial.println(totalSold[i]);
  }
  
  // Close the table tag
  html += "</table>";

  // Add the export CSV button
  html += "<form action='/exportSales' method='post'><button type='submit'>Exportiere Verkäufe als CSV</button></form>";

  // Add the reset sales button
  html += "<form action='/resetSales' method='post'><button type='submit'>Verkäufe zurücksetzen</button></form>";

  // Set the Content-Type header to UTF-8
  server.sendHeader("Content-Type", "text/html; charset=UTF-8");
  
  // Send the HTML response
  server.send(200, "text/html", html);
}


// Endpoint to handle CSV export
void handleExportSales() {
  String csvData = "Produkt,Anzahl\n";
  for (int i = 0; i < productCount; i++) {
    csvData += String(products[i].name) + "," + String(totalSold[i]) + "\n";
  }
  
  server.sendHeader("Content-Disposition", "attachment; filename=sales.csv");
  server.sendHeader("Content-Type", "text/csv");
  server.send(200, "text/csv", csvData);
}

// Endpoint to handle sales reset
void handleResetSales() {
  // Reset the sales data, you can clear the totalSold array or reset EEPROM data here
  for (int i = 0; i < productCount; i++) {
    totalSold[i] = 0; // Reset the total sales for each product
  }

  // Optionally, clear EEPROM or reset any other related variables
  EEPROM.begin(512); // Make sure EEPROM is initialized
  for (int i = 0; i < productCount; i++) {
    EEPROM.write(i, 0); // Reset EEPROM data for sales
  }
  EEPROM.commit(); // Make sure the changes are saved to EEPROM

  // Redirect to the sales overview page after resetting
  server.sendHeader("Location", "/sales"); // Redirect to the sales  page
  server.send(303); // Send a redirect response
  // wait for 1 second before restarting
  delay(1000); // Optional delay before restarting
  ESP.restart(); // Restart the ESP32 to apply changes
}



// HTML for product page
String generateProductList() {
  String content = "";

  // repeated for the number of products in the shop
  for (int i = 0; i < productCount; i++) {
    content += "<div class='product'>";
    content += "<p><strong>" + String(products[i].name) + "</strong> (" + String(products[i].price, 2) + " €";
    if (products[i].hasDeposit) content += " + 1 € Pfand";
    content += ")</p>";
    content += "<div class='row'><div class='left'>";
    content += "<span>Anzahl: " + String(products[i].count) + "</span>";

    // add product buttons
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 1)' style='background-color: green; color: white;'>+1</button>"; // +1 Button
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 2)' style='background-color: green; color: white;'>+2</button>"; // +2 Button
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 3)' style='background-color: green; color: white;'>+3</button>"; // +3 Button

    content += "</div>";

    // -1 button on the right side of the row
    content += "<button onclick='sendAction(\"remove\", " + String(i) + ")' style='background-color: red; color: white;'>-1</button>";

    content += "</div>"; // line end
    content += "</div>"; // product block end
  }

  content += "<h3>Gesamtpreis: " + String(calculateTotal(), 2) + " €<br>";
  content += "<small>(inkl. " + String(calculateDeposit(), 2) + " € Pfand)</small></h3>";
  content += "<button onclick='sendAction(\"clear\", -1)'>Warenkorb löschen</button>";

  return content;
}

// configuration page HTML
String generateConfigPage() {
  // HTML template for the configuration page
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Konfiguration</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          padding: 20px;
          max-width: 600px;
          margin: auto;
        }
        h1, h2 {
          text-align: center;
        }
        .product-config {
          border: 1px solid #ccc;
          border-radius: 15px;
          padding: 15px;
          margin-bottom: 10px;
          background-color: #f9f9f9;
        }
        label {
          display: block;
          margin-top: 8px;
        }
        input[type='text'], input[type='number'] {
          width: 100%;
          padding: 8px;
          margin-top: 4px;
          border-radius: 5px;
          border: 1px solid #ccc;
        }
        input[type='checkbox'] {
          margin-top: 6px;
        }
        button, input[type='submit'] {
          margin-top: 10px;
          padding: 8px 15px;
          border: none;
          border-radius: 10px;
          background-color: #007BFF;
          color: white;
          cursor: pointer;
        }
        button:hover, input[type='submit']:hover {
          background-color: #0056b3;
        }
        hr {
          margin-top: 20px;
        }
      </style>
    </head>
    <body>
    )rawliteral";    
  html += "<style>.input-field { width: 90%; box-sizing: border-box; }</style>";  // CSS fix for input fields to be 90% of the page width
  html += "<h1>Produktkonfiguration</h1><form method='POST' action='/saveConfig'>";
  // repeated for the number of products in the shop, adding the product name, price and deposit checkbox for each product
  for (int i = 0; i < productCount; i++) {
    html += "<div class='product-config'>";
    html += "<label>Name </label>";
    html += "<input class='input-field' type='text' name='name_" + String(i) + "' value='" + String(products[i].name) + "'><br>";
    html += "<label>Preis </label>";
    html += "<input class='input-field' type='number' step='0.01' name='price_" + String(i) + "' value='" + String(products[i].price, 2) + "'><br>";
    html += "<div style='display: flex; justify-content: space-between; align-items: center;'>";
    html += "<label>Pfand <input type='checkbox' name='deposit_" + String(i) + "'" + (products[i].hasDeposit ? " checked" : "") + "></label>";
    html += "<button type='button' style='background-color: red; color: white;' onclick='deleteProduct(" + String(i) + ")'>Produkt löschen</button>";
    html += "</div>"; // End of flex line
    html += "</div>"; // end of product config block
  }

  
  // Section for new Product at the end of the page
  html += "<h2>Neues Produkt</h2>";
  html += "<label>Name</label><input class='input-field' type='text' name='new_name'><br>";
  html += "<label>Preis</label><input class='input-field' type='number' step='0.01' name='new_price'><br>";
  html += "<label>Pfand<input type='checkbox' name='new_deposit'></label><br>";
  html += "<input type='submit' value='Speichern'></form>";

  html += "<script>function deleteProduct(id){fetch('/deleteProduct?id='+id).then(()=>location.reload());}</script>"; // delete product script for button (references the function in the HTML))

  // footer with copyright 
  html += "<footer style='text-align: center; margin-top: 20px; font-size: 12px; color: #888;'>";
  html += "&copy; 2025 Imanuel Fehse | Alle Rechte vorbehalten.";
  html += "</footer>";
  html += "</body></html>";
  return html;
}


// WEB SERVER HANDLER FUNCTIONS
// Port 80 product page
void handleRoot() {
  // HTML template for the product page
  loadSalesFromEEPROM(); // load sales from EEPROM
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kasse</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        padding: 20px;
        max-width: 600px;
        margin: auto;
      }
      h1, h3 {
        text-align: center;
      }
      .product {
        border: 1px solid #ccc;
        border-radius: 15px;
        padding: 15px;
        margin-bottom: 10px;
        background-color: #f9f9f9;
      }
      .row {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-top: 10px;
      }
      .left {
        display: flex;
        align-items: center;
        gap: 10px;
      }
      button {
        font-size: 16px;
        padding: 5px 10px;
        margin-left: 5px;
        border-radius: 10px;
        border: none;
        color: white;
        cursor: pointer;
      }

      .button-green {
        background-color: green;
      }

      .button-red {
        background-color: red;
      }

      button:hover {
        background-color:rgb(116, 116, 116);
      }
      .clear-button {
        width: 100%;
        padding: 10px;
        background-color: red;
        color: white;
        font-size: 18px;
        border-radius: 10px;
        border: none;
        margin-top: 20px;
      }
    </style>
    <script>
      function updateContent(){
        fetch('/content').then(response => response.text()).then(html => {
          document.getElementById('content').innerHTML = html;
        });
      }

      function sendAction(action, id, quantity = 1){
        fetch(`/${action}?id=${id}&quantity=${quantity}`).then(() => updateContent());
      }

      window.onload = function() {
        updateContent();
      }
    </script>
  </head>
  <body>
    <h1>Kassensystem</h1>
    <div id="content">
      Lade Produkte...
    </div>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html); // send HTML to client
}

// add, remove, clear product functions
void handleAdd() {
  int id = server.arg("id").toInt();
  int q = server.arg("quantity").toInt();
  if (id >= 0 && id < productCount) products[id].count += q;
  server.send(200, "text/plain", "OK");
}

void handleRemove() {
  int id = server.arg("id").toInt();
  if (id >= 0 && id < productCount && products[id].count > 0) products[id].count--;
  server.send(200, "text/plain", "OK");
}

void handleClear() {
  for (int i = 0; i < productCount; i++) products[i].count = 0;
  server.send(200, "text/plain", "OK");
}

void handleSubmit() {
  for (int i = 0; i < productCount; i++) {
    totalSold[i] += products[i].count;
    products[i].count = 0;
  }
  saveSalesToEEPROM();
  server.send(200, "text/plain", "OK");

  // Verkäufe aktualisieren (already handled above)

  saveSalesToEEPROM(); // Deine Funktion zum Speichern der Verkäufe
}


// update content of product page when action was performed by client (add, remove, clear)
void handleContent() {
  String content = "";
  for (int i = 0; i < productCount; i++) {
    content += "<div class='product'>";
    content += "<p><strong>" + String(products[i].name) + "</strong> (" + String(products[i].price, 2) + " €";
    if (products[i].hasDeposit) content += " + 1 € Pfand";
    content += ")</p>";
    content += "<div class='row'><div class='left'>";
    content += "<span>Anzahl: " + String(products[i].count) + "</span>";
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 1)' class='button-green'>+1</button>";
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 2)' class='button-green'>+2</button>";
    content += "<button onclick='sendAction(\"add\", " + String(i) + ", 3)' class='button-green'>+3</button>";
    content += "</div>";
    content += "<button onclick='sendAction(\"remove\", " + String(i) + ")' class='button-red'>-1</button>";
    content += "</div></div>";
  }

  content += "<h3>Gesamtpreis: " + String(calculateTotal(), 2) + " €<br>";
  content += "<small>(inkl. " + String(calculateDeposit(), 2) + " € Pfand)</small></h3>";
  // submit button to finalize the order and send it to the server to be saved to EEPROM
  content += "<button onclick='sendAction(\"submit\", -1)' style='background-color: blue; color: white; width: 100%; padding: 15px; font-size: 1.2em; margin-top: 10px;'>Bestellung abschließen</button>";

  // clear button to clear the cart
  content += "<div style='text-align: center; margin-top: 10px;'>";
  content += "<button class='clear-button' onclick='sendAction(\"clear\", -1)' style='padding: 8px 16px; font-size: 1em;'>Warenkorb löschen</button>";
  content += "</div>";

  content +="<footer style='text-align: center; margin-top: 20px; font-size: 12px; color: #888;'>";
  content += "&copy; 2025 Imanuel Fehse | Alle Rechte vorbehalten.";
  content += "</footer>";

  server.send(200, "text/html", content);
}





// Port 8080 configuration page
void handleConfig() {
  configServer.send(200, "text/html", generateConfigPage()); // send HTML to client
}

// save configuration page
// save new product to EEPROM and update product list
void handleSaveConfig() {
  for (int i = 0; i < productCount; i++) {
    if (configServer.hasArg("name_" + String(i))) {
      String name = configServer.arg("name_" + String(i));
      name.toCharArray(products[i].name, sizeof(products[i].name));
      products[i].price = configServer.arg("price_" + String(i)).toFloat();
      products[i].hasDeposit = configServer.hasArg("deposit_" + String(i));
    }
  }
  if (configServer.hasArg("new_name") && configServer.arg("new_name").length() > 0 && productCount < MAX_PRODUCTS) {
    String name = configServer.arg("new_name");
    name.toCharArray(products[productCount].name, sizeof(products[productCount].name));
    products[productCount].price = configServer.arg("new_price").toFloat();
    products[productCount].hasDeposit = configServer.hasArg("new_deposit");
    products[productCount].count = 0;
    productCount++;
  }
  saveProductsToEEPROM();
  configServer.sendHeader("Location", "/");
  configServer.send(303);
}

// delete product from EEPROM and update product list
void handleDeleteProduct() {
  int id = configServer.arg("id").toInt();
  if (id >= 0 && id < productCount) {
    for (int i = id; i < productCount - 1; i++) {
      products[i] = products[i + 1];
    }
    productCount--;
    saveProductsToEEPROM();
  }
  configServer.send(200, "text/plain", "OK");
}


// SETUP
void setup() {
  // Onboard LED for status
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Serial and Wifi Module
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  loadProductsFromEEPROM();
  if (productCount == 0) {
    for (int i = 0; i < defaultProductCount && i < MAX_PRODUCTS; i++) {
      products[i] = defaultProducts[i];
    }
    productCount = defaultProductCount;
    saveProductsToEEPROM();
    saveSalesToEEPROM();  // Verkäufe auch initialisieren
  }

  loadSalesFromEEPROM(); // Beispiel-Funktion, die du schreiben musst


  // Port 80
  server.on("/", handleRoot);
  server.on("/add", handleAdd);
  server.on("/remove", handleRemove);
  server.on("/clear", handleClear);
  server.on("/content", handleContent);
  server.on("/submit", handleSubmit);
  server.on("/sales", handleSalesOverview);
  server.on("/resetSales", HTTP_POST, handleResetSales);
  server.on("/exportSales", HTTP_POST, handleExportSales);
  server.onNotFound([]() {
    server.send(404, "text/plain", "404 Not Found");
  });

  
  


  // Port 8080
  configServer.on("/", handleConfig);
  configServer.on("/saveConfig", HTTP_POST, handleSaveConfig);
  configServer.on("/deleteProduct", handleDeleteProduct);

  server.begin();       // launch product page server so client can request page
  configServer.begin(); // launch config page server so client can request page
  Serial.println("servers started");

  Serial.println("product page running on port 80");
  Serial.println("config page running on port 8080");
}


// LOOP
void loop() {
  // Status LED, not blocking webservers so client action is not delayed
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    digitalWrite(LED_PIN, HIGH);  // LED an
    ledOn = true;
  }

  // LED kurz anlassen (z. B. 100 ms), dann wieder aus
  if (ledOn && millis() - previousMillis >= 100) {
    digitalWrite(LED_PIN, LOW);   // LED aus
    ledOn = false;
  }

  // Webservers looking for client requests
  server.handleClient();        // product page client handler
  configServer.handleClient();  // config page client handler
}
