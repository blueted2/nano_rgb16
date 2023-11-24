const express = require('express');
const NodeWebcam = require("node-webcam");
const path = require('path'); // Import the 'path' module
const Jimp = require("jimp");

const app = express();
const port = 3000;

// Webcam options
var opts = {
  quality: 100,
  delay: 0,
  saveShots: true,
  output: "jpeg",
  device: false,
  callbackReturn: "location",
  verbose: false
};

// Creates webcam instance
const Webcam = NodeWebcam.create(opts);

const absolutePathJpeg = path.join(__dirname, "image.jpg");
const absolutePathBmp = path.join(__dirname, "image.bmp");

// Endpoint to capture and send an image
app.get('/capture', (req, res) => {
  console.log("got request");
  Webcam.capture(absolutePathJpeg, function(err, data) {
    if (err) {
      console.error(err);
      res.status(500).send('Error capturing image');
      return;
    }

    Jimp.read(absolutePathJpeg, function(err, image) {
      if (err) {
        console.log(err)
      } else {

        image.cover(120, 100).write(absolutePathBmp);
        // Send the file using an absolute path
        res.sendFile(absolutePathBmp);
      }
    })

  });
});

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}`);
});
