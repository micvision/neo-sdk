#!/usr/bin/env python
# coding=utf-8

import itertools, threading
import sys, math
import neopy

def main():
    ### USAGE: python -m neopy /dev/ttyACM0
    if len(sys.argv) < 2:
        sys.exit('python -m neopy /dev/ttyACM0')

    dev = sys.argv[1]
    ### Named a device as neo_device
    with neopy.neo(dev) as neo_device:
        ### Set motor speed: 5
        neo_device.set_motor_speed(5)
        ### Get temp motor speed
        # speed = neo_device.get_motor_speed()

        ### Print out speed
        # print('Motor Speed: {} Hz'.format(speed))
        ### Device start scanning, and get data
        neo_device.start_scanning()

        ### Print out the scan data
        for scan in itertools.islice(neo_device.get_scans(), 10):
            print(scan)

        ### Device stop scanning
        neo_device.stop_scanning()
        ### Device set motor speed 0 to stop the motor
        neo_device.set_motor_speed(0)

import wx
from matplotlib.figure import Figure
import matplotlib.font_manager as font_manager
import numpy as np
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigureCanvas


TIMER_ID = wx.NewId()
x = [None] * 360
y = [None] * 360
class AngleCircle(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, None, style = wx.SYSTEM_MENU | wx.CAPTION | wx.CLOSE_BOX, title='Neo LiDAR demo(python)', size=(800, 800))
        self.fig = Figure((8, 8), 100)
        self.canvas = FigureCanvas(self, wx.ID_ANY, self.fig)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_ylim([-10, 10])
        self.ax.set_xlim([-10, 10])
        self.ax.set_autoscale_on(False)
        self.ax.set_xticks(range(-10, 11, 2))
        self.ax.set_yticks(range(-10, 11, 2))
        self.ax.grid(True)
#        self.datax = [None] * 360
#        self.datay = [None] * 360
        #for i in range(360):
            #x[i] = np.random.randint(-40, 40)
            #y[i] = np.random.randint(-40, 40)
        #self.datax = np.random.randn(100)
        #self.datay = np.random.randn(100)
        self.draw_data, = self.ax.plot(x, y, '.', ms = 3.0, mec = 'RED')
        self.canvas.draw()
        self.bg = self.canvas.copy_from_bbox(self.ax.bbox)
        wx.EVT_TIMER(self, TIMER_ID, self.onTimer)
        th.start()


    def onTimer(self, evt):
        global x, y
        global flag
        self.canvas.restore_region(self.bg)
        #self.draw_data.set_data(x, y)
        #self.draw_data.set_ydata(y)
        self.ax.draw_artist(self.draw_data)
        self.canvas.blit(self.ax.bbox)

class updateData(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

    def run(self):
        global x, y
        while True:
            scan = neo_device.get_scans()
            x = [None] * 360
            y = [None] * 360
            for i, s in enumerate(scan.next()[0]):
                if i >= 360: break
                angle_rad = s.angle / 1000.0 * math.pi / 180.0
                distance = s.distance / 100.0
                x[i] = distance * math.cos(angle_rad)
                y[i] = distance * math.sin(angle_rad)
                # print "angle: ", angle_rad, " , distance: ", distance
                frame.draw_data.set_data(x, y)
                #self.datax[i] = np.random.randint(-40, 40)
                #self.datay[i] = np.random.randint(-40, 40)

neo_device = neopy.neo('/dev/ttyACM0')
neo_device.set_motor_speed(5)
neo_device.start_scanning()

# threading
th = updateData()


# app
app = wx.App()
frame = AngleCircle()
t = wx.Timer(frame, TIMER_ID)
t.Start(50)
frame.Show()
app.MainLoop()

neo_device.stop_scanning()
neo_device.set_motor_speed(0)

# main()
#viewer()
