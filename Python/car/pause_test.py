#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import AutonomySim
import time
import numpy as np

# connect to the AutonomySim simulator
client = AutonomySim.CarClient()
client.confirmConnection()
client.enableApiControl(True)
car_controls = AutonomySim.CarControls()

# set the controls for car
car_controls.throttle = -0.5
car_controls.is_manual_gear = True
car_controls.manual_gear = -1
client.setCarControls(car_controls)

# let car drive a bit
time.sleep(10)

client.simPause(True)
car_position1 = client.getCarState().kinematics_estimated.position
img_position1 = client.simGetImages([AutonomySim.ImageRequest(0, AutonomySim.ImageType.Scene)])[0].camera_position
print(f"Before pause position: {car_position1}")
print(f"Before pause diff: {car_position1.x_val - img_position1.x_val}, {car_position1.y_val - img_position1.y_val}, {car_position1.z_val - img_position1.z_val}")

time.sleep(10)

car_position2 = client.getCarState().kinematics_estimated.position
img_position2 = client.simGetImages([AutonomySim.ImageRequest(0, AutonomySim.ImageType.Scene)])[0].camera_position
print(f"After pause position: {car_position2}")
print(f"After pause diff: {car_position2.x_val - img_position2.x_val}, {car_position2.y_val - img_position2.y_val}, {car_position2.z_val - img_position2.z_val}")
client.simPause(False)
