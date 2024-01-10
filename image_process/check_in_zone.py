import numpy as np
import cv2 as cv

green_range = ((0, 10, 0), (30, 200, 30))
purple_range = ((30, 0, 30), (200, 30, 200))


def image_as_block(image:np.array, size:int, thr:float, max_count:int=100)->(np.array, int, int, int, int, bool):
    """D'une image binaire, retourne si quelque chose a été détécté et où

    Args:
        image (np.array): image binaire a analyser
        size (int): taille des blocks a créer
        thr (float): seuil de détéction
        max_count (int, optional): par défaut 100, le nombre minimum de blocks a détécté pour valider une présence
    Returns:
        image (np.array): image binaire avec des blocks à 0xFF là où une présence a été détécté
        x_min (int) : origine de la zone de détéction
        y_min (int) : origine de la zone de détéction
        x_max (int) : deuxième point de la zone de la zone de détéction
        y_max (int) : deuxième point de la zone de la zone de détéction
        detetct (bool) : Vrai si quelqu'un est détecté
    """
    x_min = 1e6
    y_min = 1e6
    x_max = 0
    y_max = 0
    count = 0
    for i in range(image.shape[0]//size):
        for j in range(image.shape[1]//size):
            if np.sum(image[i*size:(i+1)*size, j*size:(j+1)*size]):
                image[i*size:(i+1)*size, j*size:(j+1)*size] = 0xFF
                if x_min > i :
                    x_min = i
                if y_min > j :
                    y_min = j
                if x_max < i :
                    x_max = i
                if y_max < j :
                    y_max = j
                count += 1
            else:
                image[i*size:(i+1)*size, j*size:(j+1)*size] = 0
            
    return image, x_min*size, y_min*size, x_max*size, y_max*size, not (x_min>x_max) and count > max_count


def check_in(zone:np.array) ->  (bool, bool):
    """Vérifie si une personne de type verte ou violette 

    Args:
        zone (np.array): zone a analyser

    Returns:
        is_green: Vrai si détece une personne verte
        is_purple: Vrai si détece une personne violette
    """
    green_mask = cv.medianBlur(cv.inRange(zone, *green_range), 17)
    green, x_min, y_min, x_max, y_max, is_green = image_as_block(green_mask, 10, 10*10*.8)
       
    purple_mask = cv.medianBlur(cv.inRange(zone, *purple_range), 17)
    purple, x_min, y_min, x_max, y_max, is_purple = image_as_block(purple_mask, 10, 10*10*.8)
    
    return is_green, is_purple

def check_autorization(zone:int, is_green:bool, is_purple:bool)->bool:
    """Vérifie si la personne à l'autorisation de se trouver dans la zone

    Args:
        zone (int): numéros de la zone
        is_green (bool): accès de type vert
        is_purple (bool): accès de type violet

    Returns:
        bool: Vrai si la personne à le droit d'être présent dans la zone
    """
    if zone == 1:
        return is_green or is_purple
    elif zone == 2:
        return is_purple
    return False



def who_it_is(is_green:bool, is_purple:bool):
    print(is_green,is_purple)
    if is_green:
        print("Aurélien")
        return 'Aurélien'
    elif is_purple:
        print("Clémence")
        return 'Clémence'
    else:
        print("Inconnue")
        return 'Inconnue'