from autonomysim.clients import MultirotorClient
from autonomysim.utils import SetupPath


def main():
    SetupPath()

    # this script moves the drone to a location, then rests it thousands of time
    # purpose of this script is to stress test reset API

    # connect to the AutonomySim simulator
    client = MultirotorClient()
    client.confirmConnection()
    client.enableApiControl(True)
    client.armDisarm(True)

    for idx in range(3000):
        client.moveToPositionAsync(0, 0, -10, 5).join()
        client.reset()
        client.enableApiControl(True)
        print("%d" % idx)

    # that's enough fun for now. let's quite cleanly
    client.enableApiControl(False)


if __name__ == "__main__":
    main()
