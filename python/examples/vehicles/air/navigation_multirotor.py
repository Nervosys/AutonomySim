import math
import numpy as np
import cv2

from autonomysim.types import ImageType, DrivetrainType, YawMode
from autonomysim.utils import wait_key, SetupPath
from autonomysim.utils.convs import to_eularian_angles
from autonomysim.clients import MultirotorClient


def main():
    """
    Use OpenCCV to show new images from AutonomySim.
    """
    SetupPath()

    client = MultirotorClient()
    client.confirmConnection()
    client.enableApiControl(True)
    client.armDisarm(True)
    client.takeoffAsync().join()

    # NOTE: you must first press "1" in the AutonomySim view to turn on the depth capture

    # get depth image
    yaw = 0
    pi = 3.14159265483
    vx = 0
    vy = 0
    driving = 0
    help = False

    while True:
        # this will return png width= 256, height= 144
        result = client.simGetImage("0", ImageType.DepthVis)
        if result == "\0":
            if not help:
                help = True
                print(
                    "Please press '1' in the AutonomySim view to enable the Depth camera view"
                )
        else:
            rawImage = np.fromstring(result, np.int8)
            png = cv2.imdecode(rawImage, cv2.IMREAD_UNCHANGED)
            gray = cv2.cvtColor(png, cv2.COLOR_BGR2GRAY)

            # slice the image so we only check what we are headed into (and not what is down on the ground below us).

            top = np.vsplit(gray, 2)[0]

            # now look at 4 horizontal bands (far left, left, right, far right) and see which is most open.
            # the depth map uses black for far away (0) and white for very close (255), so we invert that
            # to get an estimate of distance.
            bands = np.hsplit(top, [50, 100, 150, 200])
            maxes = [np.max(x) for x in bands]
            min = np.argmin(maxes)
            distance = 255 - maxes[min]

            # sanity check on what is directly in front of us (slot 2 in our hsplit)
            current = 255 - maxes[2]

            if current < 20:
                client.hoverAsync().join()
                wait_key("whoops - we are about to crash, so stopping!")

            pitch, roll, yaw = to_eularian_angles(
                client.simGetVehiclePose().orientation
            )

            if distance > current + 30:
                # we have a 90 degree field of view (pi/2), we've sliced that into 5 chunks, each chunk then represents
                # an angular delta of the following pi/10.
                change = 0
                driving = min
                if min == 0:
                    change = -2 * pi / 10
                elif min == 1:
                    change = -pi / 10
                elif min == 2:
                    change = 0  # center strip, go straight
                elif min == 3:
                    change = pi / 10
                else:
                    change = 2 * pi / 10

                yaw = yaw + change
                vx = math.cos(yaw)
                vy = math.sin(yaw)
                print(
                    "switching angle", math.degrees(yaw), vx, vy, min, distance, current
                )

            if vx == 0 and vy == 0:
                vx = math.cos(yaw)
                vy = math.sin(yaw)

            print("distance=", current)
            client.moveByVelocityZAsync(
                vx,
                vy,
                -6,
                1,
                DrivetrainType.ForwardOnly,
                YawMode(False, 0),
            ).join()

            x = int(driving * 50)
            cv2.rectangle(png, (x, 0), (x + 50, 50), (0, 255, 0), 2)
            cv2.imshow("Top", png)

        key = cv2.waitKey(1) & 0xFF
        if key == 27 or key == ord("q") or key == ord("x"):
            break


if __name__ == "__main__":
    main()
