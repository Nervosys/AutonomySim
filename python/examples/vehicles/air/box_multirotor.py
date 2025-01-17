# import sys
import time

from autonomysim.types import DrivetrainType, YawMode
from autonomysim.clients import MultirotorClient
from autonomysim.utils import SetupPath


def main():
    SetupPath()

    client = MultirotorClient()
    client.confirmConnection()
    client.enableApiControl(True)
    client.armDisarm(True)
    client.takeoffAsync().join()

    print("Flying a small square box using moveByVelocityZ")

    # AutonomySim uses NED coordinates so negative axis is up.
    # z of -7 is 7 meters above the original launch point.
    z = -7

    # Fly given velocity vector for 5 seconds
    duration = 5
    speed = 1
    delay = duration * speed

    # using AutonomySim.DrivetrainType.MaxDegreeOfFreedom means we can control the drone yaw independently
    # from the direction the drone is flying.  I've set values here that make the drone always point inwards
    # towards the inside of the box (which would be handy if you are building a 3d scan of an object in the real world).
    vx = speed
    vy = 0
    print("moving by velocity vx=" + str(vx) + ", vy=" + str(vy) + ", yaw=90")
    client.moveByVelocityZAsync(
        vx,
        vy,
        z,
        duration,
        DrivetrainType.MaxDegreeOfFreedom,
        YawMode(False, 90),
    ).join()
    time.sleep(delay)
    vx = 0
    vy = speed
    print("moving by velocity vx=" + str(vx) + ", vy=" + str(vy) + ", yaw=180")
    client.moveByVelocityZAsync(
        vx,
        vy,
        z,
        duration,
        DrivetrainType.MaxDegreeOfFreedom,
        YawMode(False, 180),
    ).join()
    time.sleep(delay)
    vx = -speed
    vy = 0
    print("moving by velocity vx=" + str(vx) + ", vy=" + str(vy) + ", yaw=270")
    client.moveByVelocityZAsync(
        vx,
        vy,
        z,
        duration,
        DrivetrainType.MaxDegreeOfFreedom,
        YawMode(False, 270),
    ).join()
    time.sleep(delay)
    vx = 0
    vy = -speed
    print("moving by velocity vx=" + str(vx) + ", vy=" + str(vy) + ", yaw=0")
    client.moveByVelocityZAsync(
        vx,
        vy,
        z,
        duration,
        DrivetrainType.MaxDegreeOfFreedom,
        YawMode(False, 0),
    ).join()

    time.sleep(delay)
    client.hoverAsync().join()
    client.landAsync().join()


if __name__ == "__main__":
    main()
