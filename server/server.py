from flask import Flask, Response, request, send_file
import cv2
import threading
import time
import os

from pathlib import Path
cwd = Path(__file__).parent
goal_dir = cwd.parent
goal_dir = goal_dir.resolve()

from image_process.image_pp import image_process

app = Flask(__name__)

# Shared variable for storing the latest frame
latest_frame = None
frame_lock = threading.Lock()

# Initialize the webcam capture
cap = cv2.VideoCapture(0)

def camera_thread():
    global latest_frame
    while True:
        success, frame = cap.read()
        if not success:
            break

        with frame_lock:
            latest_frame = frame

        time.sleep(0.01)

def capture_frame(format='jpeg', width=None, height=None):
    with frame_lock:
        if latest_frame is not None:
            # Resize the frame if width and height are provided
            if width and height:
                frame = cv2.resize(latest_frame, (int(width), int(height)))
            else:
                frame = latest_frame

            # Encode the frame in the requested format
            ret, buffer = cv2.imencode(f'.{format}', frame)
            return buffer.tobytes()
        else:
            return None

@app.route('/capture')
def capture():
    # Get format and resolution from the request arguments
    format = request.args.get('format', 'jpeg')
    width = request.args.get('width')
    height = request.args.get('height')

    frame = capture_frame(format, width, height)
    if frame:
        return Response(frame, mimetype=f'image/{format}')
    else:
        return "Failed to capture image", 500
    

@app.route('/', methods = ["POST"])
def upload():

    if request.content_type != "image/jpeg":
        return "invalid format", 400
    
    with open("img.jpeg", "bw") as f:
        f.write(request.get_data())
        image_process(cv2.imread("img.jpeg"))

    return "", 200

@app.route('/', methods = ["GET"])
def download():
    if os.path.isfile("img.jpeg"):
        return send_file("img.jpeg")
    
    return "no image uploaded", 404
    

if __name__ == '__main__':
    threading.Thread(target=camera_thread, daemon=True).start()

    try:
        app.run(debug=False, threaded=True, port=3002, host="0.0.0.0")
    except KeyboardInterrupt:
        cap.release()
