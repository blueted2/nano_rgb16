#pip install secure-smtplib
#pip install twilio

import smtplib
import datetime
from pathlib import Path
from email.mime.text import MIMEText
from twilio.rest import Client
from email.mime.multipart import MIMEMultipart
from email.mime.base import MIMEBase
from email import encoders

API_TOKEN = 'token' #TODO  

"""
Liste de fonction :

send_email : Envoit du mail
send_sms : Envoit du sms
write_message : Écriture du message
prep_message : Préparation initiale (récupération date et heure)
padding : Ajustement pour l'heure

Lancer prep_message( individue, zone, recipients)
ex : prep_message("Clémence", 1, recipients)
"""


def send_email(subject, body, sender, recipients, password, image:bool=False):
    """
    Envoie un mail à la liste de destinaires dans recipients, dupuis le mail sender
    subject = sujet du mail
    body = texte du mail
    sender = mail de l'envoyeur (obligatoirment adresse gmail)
    recipients = Liste des destinataires (format liste même pour une personne)
    password = mot de passe d'application (API key) du compte gmail envoyeur
    """
    msg = MIMEMultipart()
    msg['Subject'] = subject
    msg['From'] = sender
    msg['To'] = ', '.join(recipients)

    msg.attach(MIMEText(body))

    if image : 
        part = MIMEBase('application', "octet-stream")
        with open("image.jpeg", 'rb') as file:
            part.set_payload(file.read())
        encoders.encode_base64(part)
        part.add_header('Content-Disposition',
                        'attachment; filename={}'.format(Path("image.jpeg").name))
        msg.attach(part)


    with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp_server:
       smtp_server.login(sender, password)                          #login au compte google
       smtp_server.sendmail(sender, recipients, msg.as_string())    #envoie du mail
    print("Message sent!")



def send_sms(body):
    account_sid = 'ACa8e4bedc1d56fa5d97f32746adf7b600'
    auth_token = API_TOKEN
    client = Client(account_sid, auth_token)

    message = client.messages.create( from_='+12057821255', body=body, to='+33782848768')

    print(message.sid)


def write_message(recipients, date, heure, individue, zone) : 
    """
    Prépare le mail avant l'envoit 
    date et heure = date et heure (format str)   
    recipients = Liste des destinataires (format liste même pour une personne)
    individue = nom de la personne reconnue, individue = inconnue si la personne n'est pas identifié
    zone = zone de la transgression (int ou str ?)
    """

    subject = "Security mail from camera"
    sender = "grenierclemence16@gmail.com"
    password = "genw wdup ehth fmgk"

    if individue == "inconnue" :
        str = f"L'individue n'a pas pu être identifié" 
    else : 
        str = f"L'individue a été reconue, il s'agit de {individue}."

    body = f"""Bonjour,

Une personne non autorisé à été vu dans la zone {zone}, le {date} à {heure}.
{str}

Veuillez prendre les mesures nécessaire.
    """

    # --- Paramétrage des zones ---

    if zone == 3:
        send_sms(body)
        print("send sms")
    elif zone == 2 :
        send_email(subject, body, sender, recipients, password, image=True)
        print("send mail")
    elif zone == 1 :
        print("tout va bien")


def padding(liste:str, number_to_add:int, char_to_add="0", place_before=True, copy=False)->str:
    """Padd une liste

    Args:
        liste (list): liste à padder
        number_to_add (int): nombre de charactère à ajouter
        char_to_add (int, optional): charactère à ajouter. Defaults to 0.
        place_before (bool, optional): ajouter les charactères de bourrage au début ou à la fin de la liste. Defaults to True.
        copy (bool, optional): si True fait les modifie directement la liste d'entrée ou si doit créer une copy. Defaults to False.

    Returns:
        list: liste paddé
    """
    if copy:
        liste = liste.copy()
    for _ in range(number_to_add):
        # liste.insert(0 if place_before else len(liste)-1, char_to_add)
        liste = (liste + char_to_add) if place_before else (char_to_add+liste)
    return liste


# send_mail(recipients, date, heure, individue, zone)


def prep_message(individue, zone, recipients): 
    """
    récupère l'heure et la date actuelle
    individue = str indiquant la personne en transgression  
    zone = numéro de la zone de la transgression 
    recipients = Liste des destinataires (format liste même pour une personne)
    """
    now = datetime.datetime.now()
    # date = now.date()
    # heure = now.time()
    taille = 2
    hour = padding(str(now.hour), max(taille-len(str(now.hour)), 0), place_before=False)
    minute = padding(str(now.minute), max(taille-len(str(now.minute)), 0), place_before=False)
    second = padding(str(now.second), max(taille-len(str(now.second)), 0), place_before=False)


    date = f"{now.day}/{now.month}/{now.year}" #écriture de la date        
    heure = f"{hour}:{minute}:{second}"          #écriture de l'heure

    write_message(recipients, date, heure, individue, zone)

    
def alert_acc(recipients):

    now = datetime.datetime.now()
    taille = 2
    hour = padding(str(now.hour), max(taille-len(str(now.hour)), 0), place_before=False)
    minute = padding(str(now.minute), max(taille-len(str(now.minute)), 0), place_before=False)
    second = padding(str(now.second), max(taille-len(str(now.second)), 0), place_before=False)


    date = f"{now.day}/{now.month}/{now.year}" #écriture de la date        
    heure = f"{hour}:{minute}:{second}"          #écriture de l'heure

    subject = "Security mail from camera"
    sender = "grenierclemence16@gmail.com"
    password = "genw wdup ehth fmgk"

    body = f"""Bonjour,

La caméra à subie un mouvement important le {date} à {heure}.
L'intégrité du système est compromise. 

Veuillez prendre les mesures nécessaire.
    """
    send_email(subject, body, sender, recipients, password)


#---------main------------
if __name__ == '__main__':
    recipients = ["grenierclemence16@gmail.com" ] # Liste des recepteurs du mail
    prep_message("Clémence", 1, recipients)
    alert_acc(recipients)



