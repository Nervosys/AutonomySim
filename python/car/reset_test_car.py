import setup_path
import autonomysim

import time

# connect to the AutonomySim simulator
client = autonomysim.CarClient()
client.confirmConnection()
client.enableApiControl(True)
client.armDisarm(True)
car_controls = autonomysim.CarControls()

# go forward
car_controls.throttle = 1
car_controls.steering = 1
client.setCarControls(car_controls)
print("Go Forward")
time.sleep(5)  # let car drive a bit

print("reset")
client.reset()
time.sleep(5)  # let car drive a bit

client.setCarControls(car_controls)
print("Go Forward")
time.sleep(5)  # let car drive a bit
