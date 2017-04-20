# -*- coding: utf-8 -*-
"""
Created on Wed Apr 05 17:10:19 2017

@author: Luca
"""
import sys
import serial # import Serial Library
import drawnow as drawnow
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

x_eval = np.linspace(0, 1024, 1024)



def Connect(channel, channel2):
    
    ch = 0
    i = 0
    arduinoData = serial.Serial('com4', 115200) #Creating our serial object named arduinoData
    plt.ion() #Tell matplotlib you want interactive mode to plot live data
    while True: # While loop that loops forever
    
        while (arduinoData.inWaiting()==0): #Wait here until there is data
            pass #do nothing
        try:
            arduinoString = arduinoData.readline() #read the line of text from the serial port
            print i 
            ch = int(arduinoString)            #Convert first element to floating number and put in temp
            channel[ch] = channel[ch] + 1
            channel = running_mean(channel, 4)
            if (i >= 1000):
                i = 0
                np.savetxt('counts.out', channel, delimiter=' ')   # X is an array
            drawnow.drawnow(makeFig)
            plt.pause(.00000001)                     #Pause Briefly. Important to keep drawnow from crashing
            i =i+1
        except ValueError:
            print"read wrong value"



### Running mean/Moving average
def running_mean(x, N):
    b =np.zeros((len(x),), dtype=np.float)
    a = np.linspace(1, len(x), num=len(x), endpoint=True)
    c = (N/2.0) * np.linspace(1, int(len(x)/N), num=int(len(x)/N), endpoint=True)
    f = np.linspace(int(-N/2),int(N/2), num=int(len(x)/N), endpoint=False)
    for i in range(0,int(len(x)/N)):
        for z in range(0, N):
            b[i] += x[(f[z]*i)]
        b[i] = b[i]/N
    #d = interp1d(a, b, kind='cubic')
    return (b)


def makeFig():
    
    plt.title('Channel from Arduino')
    plt.grid(True)
    plt.ylabel('Counts')
    plt.ylim(0,float(np.amax(channel)*1.15))   
    plt.xlim(0,len(channel))
    plt.grid(True) 
    plt.plot(channel, 'rx-', label='Channels')
    plt.legend(loc='upper right')
    


print("R for random, S for serial inup")
mode = raw_input("Please enter something: ")
channel = np.zeros((1024,), dtype=np.int)
channel2 = np.zeros((1024,), dtype=np.float)
while (True):
    if (mode == "R"):

        mode = raw_input("Please enter something: ")
        break
    elif (mode == "S"):   
        Connect(channel, channel2)
        break
    else:
        print("wrong input")
        mode = raw_input("Please enter something: ")
        break
    