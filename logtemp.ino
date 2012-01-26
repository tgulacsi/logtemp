
#ifdef DEBUG
#define debug(A) Serial.println(A) 
#else
#define debug(A) 
#endif

#include "SPI.h"

#include "Ethernet.h"
#include "WebServer.h"

#include <PString.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ 
  obj.print(arg); 
  return obj; 
}

#define ONE_WIRE_BUS 2
#define MAX_DEVNUM 8
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
int devnum = -1;
uint8_t devices[MAX_DEVNUM][8];

/* CHANGE THIS TO YOUR OWN UNIQUE VALUE.  The MAC number should be
 * different from any other devices on your network or you'll have
 * problems receiving packets. */
static uint8_t mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


/* CHANGE THIS TO MATCH YOUR HOST NETWORK.  Most home networks are in
 * the 192.168.0.XXX or 192.168.1.XXX subrange.  Pick an address
 * that's not in use and isn't going to be automatically allocated by
 * DHCP from your router. */
static uint8_t ip[] = { 
  192, 168, 1, 210 };
/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
#define PREFIX ""
char heading_buf[256];
PString heading = PString(heading_buf, sizeof(heading_buf));
DeviceAddress sensor_addresses[MAX_DEVNUM];
WebServer webserver(PREFIX, 8081);


void printAddress(Print &obj, DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    obj.print(deviceAddress[i], HEX);
  }
}

void print_temperatures(Print &printer) {
  if(devnum > 0) {
    debug("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    debug("DONE");

    for(int i=0; i<devnum; i++) {
      if(devices[i] != (uint8_t)NULL) {
        //printer << sensors.getTempCByIndex(i); 
        // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
        printAddress(printer, devices[i]);
        printer << '=' << sensors.getTempC(devices[i]) << '\n';
      }
    }
  } 
  else { 
    printer << "no data";
    Serial << "!\n";
  }
  printer << '\n'
    ;
}

/* commands are functions that get called by the webserver framework
 * they can read any posted data from client, and they output to the
 * server to send data back to the web browser. */
void reportCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  /* this line sends the standard "we're all OK" headers back to the
   browser */
  server.httpSuccess("text/plain; charset=utf-8");

  /* if we're handling a GET or POST, we can output our data here.
   For a HEAD request, we just stop after outputting headers. */
  if (type != WebServer::HEAD) {
    /* this defines some HTML text in read-only memory aka PROGMEM.
     * This is needed to avoid having the string copied to our limited
     * amount of RAM. */
    if(devnum <= 0) {
      server << "No devices found!";
    } 
    else {
      server << heading;

      print_temperatures(server);
    }
  }
}

void setup()
{
  char logfn[13];
  uint8_t n;

  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature me

    devnum = sensors.getDeviceCount();
  if(devnum > MAX_DEVNUM) devnum = MAX_DEVNUM;

  Serial.begin(9600); 
  Serial << "***\n";    

  DeviceAddress addr;
  for(uint8_t i = 0; i < MAX_DEVNUM; i++) {
    if(i < devnum) {
      sensors.getAddress(addr, i);
      //heading << (i+1) << ": ";
      //printAddress(heading, addr);
      //heading << "\n";
      for(int j=0; j<8; j++)
        devices[i][j] = addr[j];
    } 
    else {
      for(int j=0; j<8; j++)
        devices[i][j] = (uint8_t)NULL;
    }
  }
  print_temperatures(Serial);
  Serial << '\n';

  /* initialize the Ethernet adapter */
  Ethernet.begin(mac, ip);

  /* setup our default command that will be run when the user accesses
   * the root page on the server */
  webserver.setDefaultCommand(&reportCmd);

  /* run the same command if you try to load /index.html, a common
   * default page name */
  webserver.addCommand("index.html", &reportCmd);

  /*
  heading.print(devnum, DEC);
   heading << " devices present, parasite power is " 
   << (sensors.isParasitePowerMode() ? "ON" : "OFF") << "\n";
   */
  /*
  heading << (const char*)"\n";
   heading << "[" << strlen(heading) << "]\n";
   Serial.println(heading);
   */

  /* start the webserver */
  webserver.begin();
}

void loop()
{
  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
}

