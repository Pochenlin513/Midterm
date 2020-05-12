import numpy as np
import serial
import time

waitTime = 0.1

# generate the waveform table
signalLength = 182
LittleStar = [
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261]
TwoTigers = [
  262, 294, 330, 262, 262, 294, 330,262,
  330, 349, 392, 330, 349, 392,
  393, 440, 393, 349, 330, 262,
  393, 440, 393, 349, 330, 262,
  294, 196, 262, 294, 196, 262,]
SunnyDay = [
  294, 392, 392, 494, 523, 494, 440,
  392, 440, 494, 494, 494, 494,
  440, 494, 440, 392,]
LSLength = [
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2]
TTLength = [
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 4, 2, 2, 4,
  1, 1, 1, 1, 2, 2,
  1, 1, 1, 1, 2, 2,
  2, 2, 4,
  2, 2, 4]
SDLength = [
  2, 2, 2, 2, 2, 2, 2,
  1, 1, 2, 2, 2, 2,
  1, 1, 2, 2]
formatter = lambda x: "%d" % x

# send the waveform table to K66F
serdev = '/dev/ttyACM0'
s = serial.Serial(serdev)
print("Sending signal ...")
print("It may take about %d seconds ..." % (int(signalLength * waitTime)))
for data in LittleStar:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)

for data in TwoTigers:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)

for data in SunnyDay:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)

for data in LSLength:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)

for data in TTLength:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)

for data in SDLength:
  s.write(bytes(formatter(data), 'UTF-8'))
  print(bytes(formatter(data), 'UTF-8'))
  time.sleep(waitTime)
s.close()
print("Signal sended")