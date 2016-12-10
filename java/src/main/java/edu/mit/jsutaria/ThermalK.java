package edu.mit.jsutaria;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Scanner;

import org.sintef.jarduino.AnalogPin;
import org.sintef.jarduino.DigitalPin;
import org.sintef.jarduino.DigitalState;
import org.sintef.jarduino.JArduino;
import org.sintef.jarduino.PinMode;

@SuppressWarnings("deprecation")
public class ThermalK extends JArduino {
	
	public static void main(String[] args) {
		ThermalK tk = new ThermalK("COM1", 6.2675, 0.01, 0.01, 0.0019635);
		tk.runArduinoProcess();
	}
	
	public ThermalK(String port, double q, double l_1, double l_2, double a) {
		super(port);
		this.Q = q;
		this.L_1 = l_1;
		this.L_2 = l_2;
		this.A = a;
	}
	
	public ThermalK(String port) {
		super(port);
		this.Q = 6.2675;
		this.L_1 = 0.01;
		this.L_2 = 0.01;
		this.A = 0.0019635;
	}
	
	public final boolean SER = true; //set true to enable serial output support
	public final boolean RLS = true; //set true to enable relay support
	public final boolean FLE = true; //set true to enable file saving
	
	private String versProg = "0.01 - 20161103";
	private String nameProg = "ThermalK: Thermal Conductivity Monitor";
	private String nameProgShort = "ThermalK";
	private String developer = "Nicola Feralis <feranick@hotmail.com>, Jainil Sutaria <jsutaria@mit.edu>";
	
	private double Q;
	private double L_1;
	private double L_2;
	private double A;
	
	private final double THERMISTORNOMINAL = 10000.0;
	private final double TEMPERATURENOMINAL = 25.0;
	private final int NUMSAMPLES = 5;
	private final double BCOEFFICIENT = 3950.0;
	private final double SERIESRESISTOR = 10000.0;

	private final int TBits = 1023;
	private double display_delay = 0.2; 
	private double TmediumInitial = 0.0;

	private final double TtargCool = TEMPERATURENOMINAL;
	private final double minT = 100.0;

	private DigitalPin RL1 = DigitalPin.PIN_7;
	private DigitalPin RL2 = DigitalPin.PIN_6;
	private DigitalPin RL3 = DigitalPin.PIN_5;
	
	private AnalogPin therm1 = AnalogPin.A_0;
	private AnalogPin therm2 = AnalogPin.A_1;
	private AnalogPin therm3 = AnalogPin.A_2;
	
	private Path file;
	private List<String> data;
	
	private Date date;
	private long offSet;
	private long currentTime;
	@Override
	protected void setup() {
		
		
		progInfo();
		setupTime();
		setupFile();
		
		setupRelays();
		finalizeSetup();
		System.out.println("hi");
		firstSerialRun();
		menuProg();
		t.start(); // starts the external reader
		
	}
	/*
	####################################################
	Provides a reader in an external thread to allow for
	the input to be taken in despite the current state 
	the program is in
	####################################################
	*/
	public static int inSerial = 0;
	
	private Runnable r = new Runnable() {
		public void run() {
			Scanner br = new Scanner(System.in);
			while(true) {
				inSerial = br.nextInt();
				System.out.println(inSerial);
				if(inSerial == 4) break;
			}
			br.close();
		}
	};
	
	private Thread t = new Thread(r);
	
	
	private State state = State.STOPPED;
	@Override
	protected void loop() {
		currentTime = System.currentTimeMillis();
		
		switch(inSerial) {
			case 0:
				if(state.equals(State.RUNNING)) {
					acquisition();
				}
				if(state.equals(State.COOL_DOWN)) {
					cooldown();
				}
				if(state.equals(State.STOPPED)) {
					relayRamp(false, RL1);
					relayRamp(false, RL2);
					relayRamp(false, RL3);
				}
				break;
			case 1: 
				inSerial = 0;
				if(state.equals(State.STOPPED)) { 
					state = State.RUNNING;
					offSet = System.currentTimeMillis();
					setupFile();
				}
				break;
			case 2:
				inSerial = 0;
				state = State.STOPPED;
				menuProg();
				break;
			case 3:
				inSerial = 0;
				state = State.STOPPED;
				progInfo();
				menuProg();
				break;
			case 4:
				inSerial = 0;
				this.stopArduinoProcess();
				this.runArduinoProcess();
				break;
			case 5:
				inSerial = 0;
	 			if(!RLS) logln("Error, unknown input");
				else state = State.COOL_DOWN;
				break;
			default: 
				inSerial = 0;
				logln("Error, unknown input");
				break;
		}
		
	}
	
	private void acquisition() {
		relayRamp(true, RL1);
		relayRamp(true, RL2);
		relayRamp(true, RL3);
		double T1 = round(tread(therm1), 2);
		double T2 = round(tread(therm2), 2);
		double T3 = round(tread(therm3), 2); //This is for the heater
		
		double deltaT_1 = T3 - T1;
		double deltaT_2 = T3 - T2;
		double conductivity = round((Q) / (A * ((deltaT_1 / L_1) + (deltaT_2 / L_2))), 2);
		
		double timeVal = round((currentTime - offSet) / 1000.0, 1);
		String logging = timeVal + ", " + T1 + ", " + T3 + ", " + T2 + ", " + conductivity;
		logln(logging);
		saveToFile(logging);
		
		delay((long) display_delay * 1000);            // wait for display refresh
	}
	
	private void cooldown() {
	      logln("Cool Down in progress. Press (2) to stop");
	      double T1 = round(tread(therm1), 2);
	      double T2 = round(tread(therm2), 2);
	      double T3 = round(tread(therm3), 2);
	      String logging = T1 + ", " + T3 + ", " + T2;
	      logln(logging);
	      relayRamp(true, RL1);
	      relayRamp(true, RL2);
	      relayRamp(false, RL3);

	}
	

	private void menuProg() {
		logln("---------------------------------------------------------");
		log("(1) Start; (2) Stop; (3) Info; (4) Reset");
		if(RLS) log("; (5) Cool Down"); 
		logln("");
		logln("---------------------------------------------------------\n");
	}

	private void progInfo() {
		logln("------------------------------------------------------------------------");
		log(nameProg);
		log(" - v. ");
		logln(versProg);
		logln(developer);
		logln("------------------------------------------------------------------------\n");
	}
	
	private void saveToFile(String line) {
		data.add(line);
		
		try {Files.write(file, data, Charset.defaultCharset());} 
		catch (IOException e) {}
	}
	private void logln(Object message) {
		if(SER) Serial.println(message);
	}
	
	private void log(Object message) {
		if(SER) Serial.print(message);

	}
	
	private void setupRelays() {
		if(RLS) {
			logln("Support for relays: enabled");
			pinMode(RL1, PinMode.OUTPUT);
			pinMode(RL2, PinMode.OUTPUT);
			pinMode(RL3, PinMode.OUTPUT);
			relayRamp(false, RL1);
			relayRamp(false, RL2);
			relayRamp(false, RL3);
		} else {
			logln("Support for relays: disabled");
		}
	}

	private void relayRamp(boolean ramp, DigitalPin dp) {
		if(ramp) digitalWrite(dp, DigitalState.LOW);
		else digitalWrite(dp, DigitalState.HIGH);
	}
	
	private void setupFile() {
		if(!FLE) return;
	    log("Initializing File Saving... ");
	    String[] dates = date.toString().split(" ");
	    String dateString = "" + dates[5] + "." + dates[1] + "." + dates[2];
	    int n = 1;
	    while(true) {
	    	if ((new File(dateString + "-" + n + ".csv").exists())) n++;
	    	else break;
	    }
		file = Paths.get(dateString + "-" + n + ".csv");
		data = new ArrayList<String>();
		data.add("\"Time\",\"T Lower Cold Plate\",\"T Lower Hot Plate\",\"T Upper Cold Plate\",\"Thermal Conductivity\"");
		
		try {Files.write(file, data, Charset.defaultCharset());} 
		catch (IOException e) {}
		
		if(file != null) logln("Enabled!");
		else logln("Failed to enable!");
	}
	
	private void setupTime() {
	      date = new Date();
	      logln("Date/Time enabled..." + date.toString());
	}
	
	private void firstSerialRun() {
		log("Time: ");
		log(date.getHours());
		log(":");
		if(date.getMinutes() < 10) log(0);
		log(date.getMinutes());
		log(":");
		if(date.getSeconds() < 10) log(0);
		log(" (");
		log(date.getMonth());
		log("-");
		log(date.getDay());
		log("-");
		log(date.getYear());
		logln(")");
		logln("");
		
		log("Refresh (s): ");
		logln(display_delay);
		log("T Lower CP (C): ");
		logln(tread(therm1));
		log("T Lower HP (C): ");
		logln(tread(therm2));
		log("T Upper CP (C): ");
		logln(tread(therm3));
		logln("");
		
	}
	
	private void finalizeSetup() {
		log("Saving data: ");
	    logln(file.getFileName());
	    logln("");
	    logln("Using parameters from SD card:");
	    log(" Q - heat input (watts): ");
	    logln(Q);
	    log(" L_1 - thickness of sample 1: ");
	    logln(L_1);
	    log(" L_2 - thickness of sample 2: ");
	    logln(L_2);
	    log(" Target temperature for cool down (C): ");
	    logln(TtargCool);
	    logln("");
	    
	    TmediumInitial = tread(therm3);
	}
	
	private double tread(AnalogPin pin) {
		int[] samples = new int[NUMSAMPLES];
		for (int i = 0; i < NUMSAMPLES; i++) {
			samples[i] = analogRead(pin);
			delay(10);
	    }
		
		double average = 0;
		for (int i = 0; i < NUMSAMPLES; i++) average += samples[i];
		average /= NUMSAMPLES;
		
		average = TBits  / average - 1;
		average = SERIESRESISTOR / average;
		
		double steinhart;
		steinhart = average / THERMISTORNOMINAL;
		steinhart = Math.log(steinhart);
		steinhart /= BCOEFFICIENT;
		steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);
		steinhart = 1.0 / steinhart;
		steinhart -= 273.15;
		// steinhart = (steinhart*(9/5) + 32);
		
		return steinhart;
	}

	public double round(double value, int places) {
	    if (places < 0) throw new IllegalArgumentException();

	    long factor = (long) Math.pow(10, places);
	    value = value * factor;
	    long tmp = Math.round(value);
	    return (double) tmp / factor;
	}
	
}
