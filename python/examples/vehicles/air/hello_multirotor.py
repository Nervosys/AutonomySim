import os
import tempfile
import pprint
import numpy as np
import cv2

from autonomysim.types import ImageRequest, ImageType
from autonomysim.utils import wait_key, SetupPath
from python.autonomysim.utils.io import get_pfm_array, write_file, write_pfm
from autonomysim.clients import MultirotorClient


def main():
    SetupPath()

    # connect to the AutonomySim simulator
    client = MultirotorClient()
    client.confirmConnection()
    client.enableApiControl(True)

    state = client.getMultirotorState()
    s = pprint.pformat(state)
    print("state: %s" % s)

    imu_data = client.getImuData()
    s = pprint.pformat(imu_data)
    print("imu_data: %s" % s)

    barometer_data = client.getBarometerData()
    s = pprint.pformat(barometer_data)
    print("barometer_data: %s" % s)

    magnetometer_data = client.getMagnetometerData()
    s = pprint.pformat(magnetometer_data)
    print("magnetometer_data: %s" % s)

    gps_data = client.getGpsData()
    s = pprint.pformat(gps_data)
    print("gps_data: %s" % s)

    wait_key("Press any key to takeoff")
    print("Taking off...")
    client.armDisarm(True)
    client.takeoffAsync().join()

    state = client.getMultirotorState()
    print("state: %s" % pprint.pformat(state))

    wait_key("Press any key to move vehicle to (-10, 10, -10) at 5 m/s")
    client.moveToPositionAsync(-10, 10, -10, 5).join()

    client.hoverAsync().join()

    state = client.getMultirotorState()
    print("state: %s" % pprint.pformat(state))

    wait_key("Press any key to take images")
    # get camera images from the car
    responses = client.simGetImages(
        [
            ImageRequest("0", ImageType.DepthVis),  # depth visualization image
            ImageRequest(
                "1", ImageType.DepthPerspective, True
            ),  # depth in perspective projection
            ImageRequest("1", ImageType.Scene),  # scene vision image in png format
            ImageRequest("1", ImageType.Scene, False, False),
        ]
    )  # scene vision image in uncompressed RGBA array
    print("Retrieved images: %d" % len(responses))

    tmp_dir = os.path.join(tempfile.gettempdir(), "autonomysim_drone")
    print("Saving images to %s" % tmp_dir)
    try:
        os.makedirs(tmp_dir)
    except OSError:
        if not os.path.isdir(tmp_dir):
            raise

    for idx, response in enumerate(responses):
        filename = os.path.join(tmp_dir, str(idx))

        if response.pixels_as_float:
            print(
                "Type %d, size %d"
                % (response.image_type, len(response.image_data_float))
            )
            write_pfm(os.path.normpath(filename + ".pfm"), get_pfm_array(response))
        elif response.compress:  # png format
            print(
                "Type %d, size %d"
                % (response.image_type, len(response.image_data_uint8))
            )
            write_file(os.path.normpath(filename + ".png"), response.image_data_uint8)
        else:  # uncompressed array
            print(
                "Type %d, size %d"
                % (response.image_type, len(response.image_data_uint8))
            )
            img1d = np.fromstring(
                response.image_data_uint8, dtype=np.uint8
            )  # get numpy array
            img_rgb = img1d.reshape(
                response.height, response.width, 3
            )  # reshape array to 4 channel image array H X W X 3
            cv2.imwrite(os.path.normpath(filename + ".png"), img_rgb)  # write to png

    wait_key("Press any key to reset to original state")

    client.reset()
    client.armDisarm(False)

    # that's enough fun for now. let's quit cleanly
    client.enableApiControl(False)


if __name__ == "__main__":
    main()
