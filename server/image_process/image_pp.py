from .message_alerte import prep_message
from .yolov_func import yolov_func 
from .check_in_zone import check_in, check_autorization, who_it_is
import cv2 as cv

recipients = ["grenierclemence16@gmail.com" ] # Liste des recepteurs du mail




def image_process(image):
    """
    image : np.array
    process image + send alert
    """
    # cv.imwrite('image.png', image)

    result = yolov_func(image)
    for obj in result :
        is_green, is_purple = check_in(obj[0])
        flag = check_autorization(obj[1], is_green, is_purple)

        if not flag : 
            individue = who_it_is(is_green, is_purple)
            prep_message(individue, obj[1], recipients)





