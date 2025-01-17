import pprint
import cv2

from autonomysim.clients import VehicleClient
from autonomysim.types import ImageType
from autonomysim.utils import SetupPath
from autonomysim.utils.convs import string_to_uint8_array


def main():
    # connect to the AutonomySim simulator
    SetupPath()

    client = VehicleClient()
    client.confirmConnection()

    # set camera name and image type to request images and detections
    camera_name = "0"
    image_type = ImageType.Scene

    # set detection radius in [cm]
    client.simSetDetectionFilterRadius(camera_name, image_type, 200 * 100)
    # add desired object name to detect in wild card/regex format
    client.simAddDetectionFilterMeshName(camera_name, image_type, "Cylinder*")

    while True:
        rawImage = client.simGetImage(camera_name, image_type)
        if not rawImage:
            continue
        png = cv2.imdecode(string_to_uint8_array(rawImage), cv2.IMREAD_UNCHANGED)
        cylinders = client.simGetDetections(camera_name, image_type)
        if cylinders:
            for cylinder in cylinders:
                s = pprint.pformat(cylinder)
                print("Cylinder: %s" % s)

                cv2.rectangle(
                    png,
                    (int(cylinder.box2D.min.x_val), int(cylinder.box2D.min.y_val)),
                    (int(cylinder.box2D.max.x_val), int(cylinder.box2D.max.y_val)),
                    (255, 0, 0),
                    2,
                )
                cv2.putText(
                    png,
                    cylinder.name,
                    (int(cylinder.box2D.min.x_val), int(cylinder.box2D.min.y_val - 10)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (36, 255, 12),
                )

        cv2.imshow("AutonomySim", png)
        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
        elif cv2.waitKey(1) & 0xFF == ord("c"):
            client.simClearDetectionMeshNames(camera_name, image_type)
        elif cv2.waitKey(1) & 0xFF == ord("a"):
            client.simAddDetectionFilterMeshName(camera_name, image_type, "Cylinder*")

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
