import cv2
from ultralytics import YOLO
import supervision as sv
import numpy as np
import PIL


def is_center_contained_in_zone(center_object_zone, zone_coordinates):
    zone_coordinates = zone_coordinates[0].reshape(-1)
    x_condition = np.all(zone_coordinates[0] <= center_object_zone[0] <= zone_coordinates[2])
    y_condition = np.all(zone_coordinates[1] <= center_object_zone[1] <= zone_coordinates[3])

    return x_condition and y_condition

def yolov_func(input_image:np.array)->list:
    model = YOLO("models/yolov8n.pt")
    box_annotator = sv.BoxAnnotator(
        thickness=2,
        text_thickness=2,
        text_scale=1
    )
    image_height = input_image.shape[0]
    image_width = input_image.shape[1]
    image_resolution = [image_height,image_width]
    
    ZONE_POLYGON_1 = np.array([
        [0,0],
        [image_width//3,0],
        [image_width//3, image_height],    
        [0, image_height]
    ])
    ZONE_POLYGON_2 = np.array([
        [image_width//3,0],
        [2*image_width//3, 0],
        [2*image_width//3, image_height],
        [image_width//3, image_height]
    ])

    ZONE_POLYGON_3 = np.array([
        [2*image_width//3,0],
        [image_width, 0],
        [image_width, image_height],
        [2*image_width//3, image_height]
    ])
    # Define the polygon zones
    zone_1 = sv.PolygonZone(polygon=ZONE_POLYGON_1, frame_resolution_wh=(image_width,image_height))
    zone_2 = sv.PolygonZone(polygon=ZONE_POLYGON_2, frame_resolution_wh=(image_width,image_height))
    zone_3 = sv.PolygonZone(polygon=ZONE_POLYGON_3, frame_resolution_wh=(image_width,image_height))
    
    # Annotate the image with bounding boxes and detection labels
    result = model(input_image, agnostic_nms=True)[0]
    detections = sv.Detections.from_ultralytics(result)
    detections = detections[detections.class_id == 0]  # Filter only people

    labels = [
        f"{model.model.names[class_id]} {confidence:0.2f}"
        for confidence, class_id
        in zip(detections.confidence, detections.class_id)
    ]

    # image = box_annotator.annotate(scene=input_image.copy(), detections=detections, labels=labels)

    # Detect objects in zones and annotate the image with zone labels
    zone_1.trigger(detections=detections)
    zone_2.trigger(detections=detections)
    zone_3.trigger(detections=detections)
    # image = zone_annotator_1.annotate(scene=image)
    # image = zone_annotator_2.annotate(scene=image)
    # image = zone_annotator_3.annotate(scene=image)


    # Save the annotated image
    # cv2.imwrite("annotated_"+args.input_image, image)
    original_image = PIL.Image.fromarray(input_image)
    n = 1
    objs = []
    for object_zone_coordinates in detections.xyxy:
        zone1_coordinates = np.array([tuple(ZONE_POLYGON_1[0]), tuple(ZONE_POLYGON_1[2])])
        zone2_coordinates = np.array([tuple(ZONE_POLYGON_2[0]), tuple(ZONE_POLYGON_2[2])])
        zone3_coordinates = np.array([tuple(ZONE_POLYGON_3[0]), tuple(ZONE_POLYGON_3[2])])

        center_object_zone_coordinates = ((object_zone_coordinates[0]+object_zone_coordinates[2])//2,(object_zone_coordinates[1]+object_zone_coordinates[3])//2)
        is_contained_in_zone1 = is_center_contained_in_zone(center_object_zone_coordinates, [zone1_coordinates])
        is_contained_in_zone2 = is_center_contained_in_zone(center_object_zone_coordinates, [zone2_coordinates])            
        is_contained_in_zone3 = is_center_contained_in_zone(center_object_zone_coordinates, [zone3_coordinates])

        zone = 0
        if is_contained_in_zone1 and is_contained_in_zone2 and is_contained_in_zone3:
            zone = 3
            print("The box is contained within all three zones")
        elif is_contained_in_zone1:
            zone = 1
            print("The box is contained within zone 1")
        elif is_contained_in_zone2:
            zone = 2
            print("The box is contained within zone 2")
        elif is_contained_in_zone3:
            zone = 3
            print("The box is contained within zone 3")
        else:
            print("The box is not contained within any of the three zones")

        object_detected = original_image.crop(object_zone_coordinates)
        # name_new_image = args.input_image+"_object_"+str(n)+".jpeg"
        # object_detected.save(name_new_image)
        objs.append((np.asarray(object_detected), zone))
        n += 1
    return objs