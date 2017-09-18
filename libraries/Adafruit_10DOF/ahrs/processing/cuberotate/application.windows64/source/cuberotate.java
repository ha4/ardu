import processing.core.*; 
import processing.data.*; 
import processing.event.*; 
import processing.opengl.*; 

import processing.serial.*; 
import java.awt.datatransfer.*; 
import java.awt.Toolkit; 
import processing.opengl.*; 
import saito.objloader.*; 

import java.util.HashMap; 
import java.util.ArrayList; 
import java.io.File; 
import java.io.BufferedReader; 
import java.io.PrintWriter; 
import java.io.InputStream; 
import java.io.OutputStream; 
import java.io.IOException; 

public class cuberotate extends PApplet {







float roll  = 0.0F;
float pitch = 0.0F;
float yaw   = 0.0F;
float temp  = 0.0F;
float alt   = 0.0F;

OBJModel model;
Serial   port;
String   buffer = "";

public void setup()
{
  size(400, 400, OPENGL);
  frameRate(30);
  model = new OBJModel(this);
  model.load("bunny.obj");
  model.scale(20);
  
  // ToDo: Check for errors, this will fail with no serial device present
  String ttyPort = Serial.list()[0];
  port = new Serial(this, ttyPort, 115200);
  port.bufferUntil('\n');
}
 
public void draw()
{
  background(128, 128, 128);

  // Set a new co-ordinate space
  pushMatrix();

  // Turn the lights on
  lights();
  
  // Displace objects from 0,0
  translate(200, 250, 0);
  
  // Rotate shapes around the X/Y/Z axis (values in radians, 0..Pi*2)
  rotateX(radians(roll));
  rotateZ(radians(pitch));
  rotateY(radians(yaw));

  pushMatrix();
  noStroke();
  model.draw();
  popMatrix();
  popMatrix();
}

public void serialEvent(Serial p) 
{
  String incoming = p.readString();
  if ((incoming.length() > 8))
  {
    String[] list = split(incoming, " ");
    if ( (list.length > 0) && (list[0].equals("Orientation:")) ) 
    {
      roll  = PApplet.parseFloat(list[1]);
      pitch = PApplet.parseFloat(list[2]);
      yaw   = PApplet.parseFloat(list[3]);
      buffer = incoming;
    }
    if ( (list.length > 0) && (list[0].equals("Alt:")) ) 
    {
      alt  = PApplet.parseFloat(list[1]);
      buffer = incoming;
    }
    if ( (list.length > 0) && (list[0].equals("Temp:")) ) 
    {
      temp  = PApplet.parseFloat(list[1]);
      buffer = incoming;
    }
  }
}

  static public void main(String[] passedArgs) {
    String[] appletArgs = new String[] { "cuberotate" };
    if (passedArgs != null) {
      PApplet.main(concat(appletArgs, passedArgs));
    } else {
      PApplet.main(appletArgs);
    }
  }
}
