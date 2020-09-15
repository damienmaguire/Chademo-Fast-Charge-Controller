/*  This library supports ISA Scale IVT Modular current/voltage sensor device.  These devices measure current, up to three voltages, and provide temperature compensation.


    
    This library was written by Jack Rickard of EVtv - http://www.evtv.me
    copyright 2014
    You are licensed to use this library for any purpose, commercial or private, 
    without restriction.
    
*/


#include <SimpleISA.h>

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 


ISA::ISA()  // Define the constructor.
{
  
	 timestamp = millis(); 
	 debug=false;
	 debug2=false;
	 framecount=0;
	firstframe=true;
}


ISA::~ISA() //Define destructor
{
}

void ISA::begin(int Port, int speed)
{
  
 if (Port == 0)
  {
    canPort = &Can0;
	  canEnPin = 255;
	  canSpeed = speed * 1000;
  }
  else
  {
      canPort = &Can1;
	  canEnPin = 255;
	  canSpeed = speed * 1000;
      
  }

  canPort->begin(canSpeed, canEnPin);
  canPort->attachObj(this);
  attachMBHandler(0);
  //initCurrent();  
  
}



void ISA::gotFrame(CAN_FRAME *frame, int mailbox)
 
//This is our CAN interrupt service routine to catch inbound frames
{


   switch (frame->id)
     {
     case 0x511:
      
      break;
      
     case 0x521:    
      	handle521(frame);
      break;
      
     case 0x522:    
      	handle522(frame);
      break;
      
      case 0x523:    
      	handle523(frame);
      break;
      
      case 0x524:    
      	handle524(frame);
      break;
      
      case 0x525:    
      	handle525(frame);	
      break;
      
      case 0x526:    
      	handle526(frame);	
      break;
      
      case 0x527:    
      	handle527(frame);	
      break;
      
      case 0x528:
       	handle528(frame);   
        break;
     } 
       
     if(debug)printCAN(frame);
}

void ISA::handle521(CAN_FRAME *frame)  //AMperes

{	
	framecount++;
	long current=0;
    current = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    milliamps=current;
    Amperes=current/1000.0f;

     if(debug2)Serial<<"Current: "<<Amperes<<" amperes "<<milliamps<<" ma frames:"<<framecount<<"\n";
    	
}

void ISA::handle522(CAN_FRAME *frame)  //Voltage

{	
	framecount++;
	long volt=0;
    volt = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    Voltage=volt/1000.0f;
	Voltage1=Voltage-(Voltage2+Voltage3);
  if(framecount<150)
    {
      VoltageLO=Voltage;
      Voltage1LO=Voltage1;
    }
  if(Voltage<VoltageLO &&  framecount>150)VoltageLO=Voltage;
  if(Voltage>VoltageHI && framecount>150)VoltageHI=Voltage;
  if(Voltage1<Voltage1LO && framecount>150)Voltage1LO=Voltage1;
  if(Voltage1>Voltage1HI && framecount>150)Voltage1HI=Voltage1;
  
    if(debug2)Serial<<"Voltage: "<<Voltage<<" vdc Voltage 1: "<<Voltage1<<" vdc "<<volt<<" mVdc frames:"<<framecount<<"\n";
     	
}

void ISA::handle523(CAN_FRAME *frame) //Voltage2

{	
	framecount++;
	long volt=0;
    volt = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    Voltage2=volt/1000.0f;
    if(Voltage2>3)Voltage2-=Voltage3;
    if(framecount<150)Voltage2LO=Voltage2;
    if(Voltage2<Voltage2LO  && framecount>150)Voltage2LO=Voltage2;
    if(Voltage2>Voltage2HI&& framecount>150)Voltage2HI=Voltage2;
    
		
     if(debug2)Serial<<"Voltage: "<<Voltage<<" vdc Voltage 2: "<<Voltage2<<" vdc "<<volt<<" mVdc frames:"<<framecount<<"\n";
 
   	
}

void ISA::handle524(CAN_FRAME *frame)  //Voltage3

{	
	framecount++;
	long volt=0;
    volt = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    Voltage3=volt/1000.0f;
    if(framecount<150)Voltage3LO=Voltage3;
    if(Voltage3<Voltage3LO && framecount>150 && Voltage3>10)Voltage3LO=Voltage3;
    if(Voltage3>Voltage3HI && framecount>150)Voltage3HI=Voltage3;
    
     if(debug2)Serial<<"Voltage: "<<Voltage<<" vdc Voltage 3: "<<Voltage3<<" vdc "<<volt<<" mVdc frames:"<<framecount<<"\n";
}

void ISA::handle525(CAN_FRAME *frame)  //Temperature

{	
	framecount++;
	long temp=0;
    temp = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    Temperature=temp/10;
		
     if(debug2)Serial<<"Temperature: "<<Temperature<<" C  frames:"<<framecount<<"\n";
   
}



void ISA::handle526(CAN_FRAME *frame) //Kilowatts

{	
	framecount++;
	watt=0;
    watt = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    
    KW=watt/1000.0f;
		
     if(debug2)Serial<<"Power: "<<watt<<" Watts  "<<KW<<" kW  frames:"<<framecount<<"\n";
    
}


void ISA::handle527(CAN_FRAME *frame) //Ampere-Hours

{	
	framecount++;
	As=0;
    As = (frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]);
    
    AH+=(As-lastAs)/3600.0f;
    lastAs=As;

		
     if(debug2)Serial<<"Amphours: "<<AH<<"  Ampseconds: "<<As<<" frames:"<<framecount<<"\n";
    
}

void ISA::handle528(CAN_FRAME *frame)  //kiloWatt-hours

{	
	framecount++;
	
    wh = (long)((frame->data.bytes[5] << 24) | (frame->data.bytes[4] << 16) | (frame->data.bytes[3] << 8) | (frame->data.bytes[2]));
    KWH+=(wh-lastWh)/1000.0f;
	lastWh=wh;
     if(debug2)Serial<<"KiloWattHours: "<<KWH<<"  Watt Hours: "<<wh<<" frames:"<<framecount<<"\n";
   
}


void ISA::printCAN(CAN_FRAME *frame)
{
	
   //This routine simply prints a timestamp and the contents of the 
   //incoming CAN message
   
	milliseconds = (int) (millis()/1) %1000 ;
	seconds = (int) (millis() / 1000) % 60 ;
    minutes = (int) ((millis() / (1000*60)) % 60);
	hours   = (int) ((millis() / (1000*60*60)) % 24);
	sprintf(buffer,"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
	Serial<<buffer<<" ";
    sprintf(bigbuffer,"%02X %02X %02X %02X %02X %02X %02X %02X %02X", 
    frame->id, frame->data.bytes[0],frame->data.bytes[1],frame->data.bytes[2],
    frame->data.bytes[3],frame->data.bytes[4],frame->data.bytes[5],frame->data.bytes[6],frame->data.bytes[7],0);
    Serial<<"Rcvd ISA frame: 0x"<<bigbuffer<<"\n";
    
}
void ISA::initialize()
{
  

	firstframe=false;
	STOP();
	delay(700);
	for(int i=0;i<9;i++)
	{
	
Serial.println("initialization \n");
	
	outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=(0x20+i);
        outframe.data.bytes[1]=0x42;  
        outframe.data.bytes[2]=0x02;
        outframe.data.bytes[3]=(0x60+(i*18));
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;

	   canPort->sendFrame(outframe);
     
       if(debug)printCAN(&outframe);
	   delay(500);
      
       sendSTORE();
       delay(500);
     }
    //  delay(500);
      START();
      delay(500);
      lastAs=As;
      lastWh=wh;

                      
}

void ISA::STOP()
{

//SEND STOP///////

	    outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=0x34;
        outframe.data.bytes[1]=0x00;  
        outframe.data.bytes[2]=0x01;
        outframe.data.bytes[3]=0x00;
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;
		canPort->sendFrame(outframe);
     
        if(debug) {printCAN(&outframe);} //If the debug variable is set, show our transmitted frame
} 
void ISA::sendSTORE()
{

//SEND STORE///////

	outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=0x32;
        outframe.data.bytes[1]=0x00;  
        outframe.data.bytes[2]=0x00;
        outframe.data.bytes[3]=0x00;
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;
		canPort->sendFrame(outframe);
     
        if(debug)printCAN(&outframe); //If the debug variable is set, show our transmitted frame
        
}   

void ISA::START()
{
           
 //SEND START///////

	    outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=0x34;
        outframe.data.bytes[1]=0x01;  
        outframe.data.bytes[2]=0x01;
        outframe.data.bytes[3]=0x00;
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;
		canPort->sendFrame(outframe);
     
        if(debug)printCAN(&outframe); //If the debug variable is set, show our transmitted frame
}

void ISA::RESTART()
{
         //Has the effect of zeroing AH and KWH  

	    outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=0x3F;
        outframe.data.bytes[1]=0x00;  
        outframe.data.bytes[2]=0x00;
        outframe.data.bytes[3]=0x00;
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;
		canPort->sendFrame(outframe);
     
        if(debug)printCAN(&outframe); //If the debug variable is set, show our transmitted frame
}


void ISA::deFAULT()
{
         //Returns module to original defaults  

	    outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=0x3D;
        outframe.data.bytes[1]=0x00;  
        outframe.data.bytes[2]=0x00;
        outframe.data.bytes[3]=0x00;
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;
		canPort->sendFrame(outframe);
     
        if(debug)printCAN(&outframe); //If the debug variable is set, show our transmitted frame
}


void ISA::initCurrent()
{
	STOP();
	delay(500);
	
	
Serial.println("initialization \n");
	
	outframe.id = 0x411;      // Set our transmission address ID
        outframe.length = 8;       // Data payload 8 bytes
        outframe.extended = 0; // Extended addresses  0=11-bit1=29bit
        outframe.rtr=1;                 //No request
        outframe.data.bytes[0]=(0x21);
        outframe.data.bytes[1]=0x42;  
        outframe.data.bytes[2]=0x01;
        outframe.data.bytes[3]=(0x61);
        outframe.data.bytes[4]=0x00;
        outframe.data.bytes[5]=0x00;
        outframe.data.bytes[6]=0x00;
        outframe.data.bytes[7]=0x00;

		canPort->sendFrame(outframe);
     
       if(debug)printCAN(&outframe);
	delay(500);
      
       sendSTORE();
       delay(500);
    
    //  delay(500);
      START();
      delay(500);
      lastAs=As;
      lastWh=wh;                      
}

